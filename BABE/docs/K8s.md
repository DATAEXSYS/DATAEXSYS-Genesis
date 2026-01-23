Absolutely — here is the **Helm chart skeleton + Kubernetes manifests** for **Babe NaaS** (Kubernetes-first deployment).
This is a **production-ready skeleton** that you can expand and customize.

---

# ✅ **Helm Chart Skeleton (Babe NaaS)**

### Directory structure

```
helm/
  babe-naas/
    Chart.yaml
    values.yaml
    templates/
      deployment.yaml
      service.yaml
      configmap.yaml
      secret.yaml
      hpa.yaml
      ingress.yaml
      pvc.yaml
      _helpers.tpl
```

---

## 📄 `helm/babe-naas/Chart.yaml`

```yaml
apiVersion: v2
name: babe-naas
description: Babe NaaS Helm Chart
type: application
version: 0.1.0
appVersion: "0.1.0"
```

---

## 📄 `helm/babe-naas/values.yaml`

```yaml
replicaCount: 3

image:
  repository: babe/naas
  tag: "latest"
  pullPolicy: IfNotPresent

service:
  type: ClusterIP
  port: 8080

resources:
  requests:
    cpu: 500m
    memory: 512Mi
  limits:
    cpu: 2
    memory: 2Gi

autoscaling:
  enabled: true
  minReplicas: 2
  maxReplicas: 6
  targetCPUUtilizationPercentage: 70

nodeSelector: {}
tolerations: []
affinity: {}

config:
  enableTelemetry: true
  enablePolicy: true
  enableCompliance: true

database:
  host: postgres
  port: 5432
  user: babe
  password: "changeme"
  name: babe_naas

messageBroker:
  type: nats
  host: nats
  port: 4222

telemetry:
  enabled: true
  prometheus:
    enabled: true
  grafana:
    enabled: true

security:
  mtls: true
  certManager: true
```

---

## 📄 `helm/babe-naas/templates/_helpers.tpl`

```yaml
{{- define "babe-naas.fullname" -}}
{{- printf "%s-%s" .Release.Name .Chart.Name | trunc 63 | trimSuffix "-" -}}
{{- end -}}
```

---

## 📄 `helm/babe-naas/templates/configmap.yaml`

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: {{ include "babe-naas.fullname" . }}-config
data:
  config.yaml: |
    mode: naas
    telemetry:
      enabled: {{ .Values.telemetry.enabled }}
    policy:
      enabled: {{ .Values.config.enablePolicy }}
    compliance:
      enabled: {{ .Values.config.enableCompliance }}
```

---

## 📄 `helm/babe-naas/templates/secret.yaml`

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: {{ include "babe-naas.fullname" . }}-secret
type: Opaque
stringData:
  db_password: {{ .Values.database.password }}
```

---

## 📄 `helm/babe-naas/templates/deployment.yaml`

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "babe-naas.fullname" . }}
spec:
  replicas: {{ .Values.replicaCount }}
  selector:
    matchLabels:
      app: {{ include "babe-naas.fullname" . }}
  template:
    metadata:
      labels:
        app: {{ include "babe-naas.fullname" . }}
    spec:
      containers:
        - name: babe-naas
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          imagePullPolicy: {{ .Values.image.pullPolicy }}
          ports:
            - containerPort: {{ .Values.service.port }}
          env:
            - name: DB_HOST
              value: {{ .Values.database.host }}
            - name: DB_PORT
              value: "{{ .Values.database.port }}"
            - name: DB_USER
              value: {{ .Values.database.user }}
            - name: DB_NAME
              value: {{ .Values.database.name }}
            - name: DB_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: {{ include "babe-naas.fullname" . }}-secret
                  key: db_password
          volumeMounts:
            - name: config-volume
              mountPath: /etc/babe
      volumes:
        - name: config-volume
          configMap:
            name: {{ include "babe-naas.fullname" . }}-config
```

---

## 📄 `helm/babe-naas/templates/service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: {{ include "babe-naas.fullname" . }}
spec:
  type: {{ .Values.service.type }}
  ports:
    - port: {{ .Values.service.port }}
      targetPort: {{ .Values.service.port }}
  selector:
    app: {{ include "babe-naas.fullname" . }}
```

---

## 📄 `helm/babe-naas/templates/hpa.yaml`

```yaml
{{- if .Values.autoscaling.enabled }}
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: {{ include "babe-naas.fullname" . }}
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: {{ include "babe-naas.fullname" . }}
  minReplicas: {{ .Values.autoscaling.minReplicas }}
  maxReplicas: {{ .Values.autoscaling.maxReplicas }}
  metrics:
    - type: Resource
      resource:
        name: cpu
        target:
          type: Utilization
          averageUtilization: {{ .Values.autoscaling.targetCPUUtilizationPercentage }}
{{- end }}
```

---

## 📄 `helm/babe-naas/templates/ingress.yaml`

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: {{ include "babe-naas.fullname" . }}
  annotations:
    kubernetes.io/ingress.class: nginx
spec:
  rules:
    - host: babe-naas.local
      http:
        paths:
          - path: /
            pathType: Prefix
            backend:
              service:
                name: {{ include "babe-naas.fullname" . }}
                port:
                  number: {{ .Values.service.port }}
```

---

# ✅ Kubernetes Manifests (Optional, Non-Helm)

If you want **simple YAML manifests** instead of Helm, here’s the base set.

---

## 📄 `k8s/naas-deployment.yaml`

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: babe-naas
spec:
  replicas: 3
  selector:
    matchLabels:
      app: babe-naas
  template:
    metadata:
      labels:
        app: babe-naas
    spec:
      containers:
        - name: babe-naas
          image: babe/naas:latest
          ports:
            - containerPort: 8080
```

---

## 📄 `k8s/naas-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: babe-naas
spec:
  type: ClusterIP
  ports:
    - port: 8080
      targetPort: 8080
  selector:
    app: babe-naas
```

---

## 📄 `k8s/naas-configmap.yaml`

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: babe-naas-config
data:
  config.yaml: |
    mode: naas
```

---

# Next Step

If you want, I can generate:

✅ **Babe LDK Kubernetes deployment** (DaemonSet for edge nodes)
✅ **NaaS observability stack** (Prometheus + Grafana + Loki)
✅ **NaaS message broker + DB manifests** (NATS + PostgreSQL + Redis)
✅ **GitOps pipeline templates** (ArgoCD manifests)

Just say **“Next: LDK K8s”** or **“Next: Observability stack”**.
