Absolutely — below are the **Kubernetes manifests for Babe LDK (DaemonSet edge deployment)** *and* the **Observability stack (Prometheus + Grafana + Loki)**.

---

# ✅ 1) Babe LDK Kubernetes Deployment (DaemonSet)

This deployment model is designed for **edge nodes**, where each Kubernetes node runs one LDK instance.

---

## 📄 `k8s/ldk-daemonset.yaml`

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: babe-ldk
  labels:
    app: babe-ldk
spec:
  selector:
    matchLabels:
      app: babe-ldk
  template:
    metadata:
      labels:
        app: babe-ldk
    spec:
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      containers:
        - name: babe-ldk
          image: babe/ldk:latest
          imagePullPolicy: IfNotPresent
          securityContext:
            privileged: true
          env:
            - name: MODE
              value: "ldk"
            - name: CONFIG_PATH
              value: "/etc/babe/config.yaml"
          volumeMounts:
            - name: config
              mountPath: /etc/babe
      volumes:
        - name: config
          configMap:
            name: babe-ldk-config
```

---

## 📄 `k8s/ldk-configmap.yaml`

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: babe-ldk-config
data:
  config.yaml: |
    mode: ldk
    network:
      interface: eth0
      protocol: dsr
    performance:
      use_af_xdp: true
```

---

# ⚙️ Notes

* **hostNetwork: true** ensures direct access to the host network interface.
* **privileged: true** is required for AF_XDP / raw socket access (can be hardened later with capabilities).
* LDK runs on every node (DaemonSet).

---

---

# ✅ 2) Observability Stack (Prometheus + Grafana + Loki)

This stack is essential for **production-grade monitoring**, and integrates with both LDK and NaaS.

---

## 📄 `k8s/observability-namespace.yaml`

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: babe-observability
```

---

## 📄 `k8s/prometheus.yaml`

```yaml
apiVersion: monitoring.coreos.com/v1
kind: Prometheus
metadata:
  name: babe-prometheus
  namespace: babe-observability
spec:
  replicas: 2
  serviceAccountName: prometheus
  serviceMonitorSelector:
    matchLabels:
      app: babe
  resources:
    requests:
      cpu: 200m
      memory: 512Mi
    limits:
      cpu: 1
      memory: 1Gi
```

---

## 📄 `k8s/grafana.yaml`

```yaml
apiVersion: v1
kind: Deployment
metadata:
  name: babe-grafana
  namespace: babe-observability
spec:
  replicas: 1
  selector:
    matchLabels:
      app: babe-grafana
  template:
    metadata:
      labels:
        app: babe-grafana
    spec:
      containers:
        - name: grafana
          image: grafana/grafana:latest
          ports:
            - containerPort: 3000
          env:
            - name: GF_SECURITY_ADMIN_PASSWORD
              value: "changeme"
```

---

## 📄 `k8s/grafana-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: babe-grafana
  namespace: babe-observability
spec:
  type: ClusterIP
  ports:
    - port: 3000
      targetPort: 3000
  selector:
    app: babe-grafana
```

---

## 📄 `k8s/loki.yaml`

```yaml
apiVersion: v1
kind: Deployment
metadata:
  name: babe-loki
  namespace: babe-observability
spec:
  replicas: 1
  selector:
    matchLabels:
      app: babe-loki
  template:
    metadata:
      labels:
        app: babe-loki
    spec:
      containers:
        - name: loki
          image: grafana/loki:latest
          args:
            - "-config.file=/etc/loki/local-config.yaml"
          volumeMounts:
            - name: config
              mountPath: /etc/loki
      volumes:
        - name: config
          configMap:
            name: babe-loki-config
```

---

## 📄 `k8s/loki-configmap.yaml`

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: babe-loki-config
  namespace: babe-observability
data:
  local-config.yaml: |
    auth_enabled: false
    server:
      http_listen_port: 3100
    ingester:
      lifecycler:
        address: 127.0.0.1
        ring:
          kvstore:
            store: inmemory
    schema_config:
      configs:
        - from: 2020-10-24
          store: boltdb-shipper
          object_store: filesystem
          schema: v11
          index:
            prefix: index_
            period: 24h
    storage_config:
      boltdb_shipper:
        active_index_directory: /tmp/loki/index
        cache_location: /tmp/loki/cache
        shared_store: filesystem
      filesystem:
        directory: /tmp/loki/chunks
```

---

# 🔥 Optional: Prometheus ServiceMonitor (to scrape LDK & NaaS)

## 📄 `k8s/servicemonitor.yaml`

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: babe-servicemonitor
  namespace: babe-observability
  labels:
    app: babe
spec:
  selector:
    matchLabels:
      app: babe
  endpoints:
    - port: metrics
      interval: 15s
```

---

# 🔥 Next Step (Optional)

If you want, I can generate:

✅ **NaaS message broker + DB manifests**

* NATS
* PostgreSQL
* Redis

And also integrate them into **Helm chart templates**.

Just reply:

**“Next: Broker + DB manifests”**
