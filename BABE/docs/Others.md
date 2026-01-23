Great — here are the **NaaS Message Broker + Database manifests**, plus optional **Helm chart templates** for integration.

---

# ✅ 1) NaaS Message Broker + DB Manifests

These are production-grade, but still minimal and clean for you to customize.

---

## 📄 `k8s/nats.yaml` (NATS deployment)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: babe-nats
  labels:
    app: babe-nats
spec:
  replicas: 3
  selector:
    matchLabels:
      app: babe-nats
  template:
    metadata:
      labels:
        app: babe-nats
    spec:
      containers:
        - name: nats
          image: nats:latest
          ports:
            - containerPort: 4222
            - containerPort: 8222
          args: ["-js"]
```

---

## 📄 `k8s/nats-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: babe-nats
spec:
  type: ClusterIP
  ports:
    - port: 4222
      name: client
    - port: 8222
      name: monitor
  selector:
    app: babe-nats
```

---

---

## 📄 `k8s/postgres.yaml` (PostgreSQL deployment)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: babe-postgres
spec:
  replicas: 1
  selector:
    matchLabels:
      app: babe-postgres
  template:
    metadata:
      labels:
        app: babe-postgres
    spec:
      containers:
        - name: postgres
          image: postgres:15
          env:
            - name: POSTGRES_DB
              value: babe_naas
            - name: POSTGRES_USER
              value: babe
            - name: POSTGRES_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: babe-postgres-secret
                  key: postgres-password
          ports:
            - containerPort: 5432
          volumeMounts:
            - name: postgres-data
              mountPath: /var/lib/postgresql/data
      volumes:
        - name: postgres-data
          emptyDir: {}
```

---

## 📄 `k8s/postgres-secret.yaml`

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: babe-postgres-secret
type: Opaque
stringData:
  postgres-password: "changeme"
```

---

## 📄 `k8s/postgres-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: babe-postgres
spec:
  type: ClusterIP
  ports:
    - port: 5432
  selector:
    app: babe-postgres
```

---

---

## 📄 `k8s/redis.yaml` (Redis deployment)

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: babe-redis
spec:
  replicas: 1
  selector:
    matchLabels:
      app: babe-redis
  template:
    metadata:
      labels:
        app: babe-redis
    spec:
      containers:
        - name: redis
          image: redis:7
          ports:
            - containerPort: 6379
```

---

## 📄 `k8s/redis-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: babe-redis
spec:
  type: ClusterIP
  ports:
    - port: 6379
  selector:
    app: babe-redis
```

---

# ✅ 2) Helm Chart Integration (Optional)

If you want these to be part of the Helm chart, here are the templates.

---

## 📄 `helm/babe-naas/templates/nats.yaml`

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "babe-naas.fullname" . }}-nats
spec:
  replicas: 3
  selector:
    matchLabels:
      app: {{ include "babe-naas.fullname" . }}-nats
  template:
    metadata:
      labels:
        app: {{ include "babe-naas.fullname" . }}-nats
    spec:
      containers:
        - name: nats
          image: nats:latest
          ports:
            - containerPort: 4222
            - containerPort: 8222
          args: ["-js"]
```

---

## 📄 `helm/babe-naas/templates/nats-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: {{ include "babe-naas.fullname" . }}-nats
spec:
  type: ClusterIP
  ports:
    - port: 4222
      name: client
    - port: 8222
      name: monitor
  selector:
    app: {{ include "babe-naas.fullname" . }}-nats
```

---

## 📄 `helm/babe-naas/templates/postgres.yaml`

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "babe-naas.fullname" . }}-postgres
spec:
  replicas: 1
  selector:
    matchLabels:
      app: {{ include "babe-naas.fullname" . }}-postgres
  template:
    metadata:
      labels:
        app: {{ include "babe-naas.fullname" . }}-postgres
    spec:
      containers:
        - name: postgres
          image: postgres:15
          env:
            - name: POSTGRES_DB
              value: babe_naas
            - name: POSTGRES_USER
              value: babe
            - name: POSTGRES_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: {{ include "babe-naas.fullname" . }}-postgres-secret
                  key: postgres-password
          ports:
            - containerPort: 5432
          volumeMounts:
            - name: postgres-data
              mountPath: /var/lib/postgresql/data
      volumes:
        - name: postgres-data
          emptyDir: {}
```

---

## 📄 `helm/babe-naas/templates/postgres-secret.yaml`

```yaml
apiVersion: v1
kind: Secret
metadata:
  name: {{ include "babe-naas.fullname" . }}-postgres-secret
type: Opaque
stringData:
  postgres-password: "changeme"
```

---

## 📄 `helm/babe-naas/templates/postgres-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: {{ include "babe-naas.fullname" . }}-postgres
spec:
  type: ClusterIP
  ports:
    - port: 5432
  selector:
    app: {{ include "babe-naas.fullname" . }}-postgres
```

---

## 📄 `helm/babe-naas/templates/redis.yaml`

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: {{ include "babe-naas.fullname" . }}-redis
spec:
  replicas: 1
  selector:
    matchLabels:
      app: {{ include "babe-naas.fullname" . }}-redis
  template:
    metadata:
      labels:
        app: {{ include "babe-naas.fullname" . }}-redis
    spec:
      containers:
        - name: redis
          image: redis:7
          ports:
            - containerPort: 6379
```

---

## 📄 `helm/babe-naas/templates/redis-service.yaml`

```yaml
apiVersion: v1
kind: Service
metadata:
  name: {{ include "babe-naas.fullname" . }}-redis
spec:
  type: ClusterIP
  ports:
    - port: 6379
  selector:
    app: {{ include "babe-naas.fullname" . }}-redis
```

---

# 🔥 Next Step

If you want, I can also generate:

✅ **NaaS API Gateway + Ingress**
✅ **Kubernetes RBAC policies**
✅ **Cert-manager + mTLS**
✅ **GitOps ArgoCD manifests**

Just reply with what you want next.
