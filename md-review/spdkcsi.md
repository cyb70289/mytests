# SPDK CSI

## About

This repo contains SPDK CSI ([Container Storage Interface]((https://github.com/container-storage-interface/)) plugin for Kubernetes.

SPDK CSI plugin brings SPDK to Kubernetes. It provisions SPDK logical volumes on storage node dynamically and enables Pods to access SPDK storage backend through NVMe-oF or iSCSI.

Please see [SPDK CSI Design Document](https://docs.google.com/document/d/1aLi6SkNBp__wjG7YkrZu7DdhoftAquZiWiIOMy3hskY/) for detailed introduction.

## Supported platforms

This plugin conforms to [CSI Spec v1.1.0](https://github.com/container-storage-interface/spec/blob/v1.1.0/spec.md). It is currently developed and tested only on Kubernetes.

This plugin supports `x86_64` and `Arm64` architectures.

## Project status

Status: **Pre-alpha**

## Prerequisites

SPDK-CSI is currently developed and tested with `Go 1.14`, `Docker 19.03` and `Kubernetes 1.17`.

Minimal requirement: Go 1.12+(supports Go module), Docker 18.03+ and Kubernetes 1.13+(supports CSI spec 1.0).

## Setup

### Build

- `$ make all`
Build targets spdkcsi, lint, test.

- `$ make spdkcsi`
Build SPDK-CSI binary `_out/spdkcsi`.

- `$ make lint`
Lint code and scripts.
  - `$ make golangci`
Install [golangci-lint](https://github.com/golangci/golangci-lint) and perform various go code static checks.
  - `$ make yamllint`
Lint yaml files if yamllint is installed. Requires yamllint 1.10+.

- `$ make test`
Verify go modules and run unit tests.

- `$ make image`
Build SPDK-CSI docker image.

### Parameters

`spdkcsi` executable accepts several command line parameters.

| Parameter      | Type   | Description               | Default           |
| ---------      | ----   | -----------               | -------           |
| `--controller` | -      | enable controller service | -                 |
| `--node`       | -      | enable node service       | -                 |
| `--endpoint`   | string | communicate with sidecars | /tmp/spdkcsi.sock |
| `--drivername` | striig | driver name               | csi.spdk.io       |
| `--nodeid`     | string | node id                   | -                 |

## Usage

Example deployment files can be found in deploy/kubernetes directory.

| File Name            | Usage                                      |
| -------------------- | -----                                      |
| storageclass.yaml    | StorageClass of provisioner "csi.spdk.io"  |
| controller.yaml      | StatefulSet running CSI Controller service |
| node.yaml            | DaemonSet running CSI Node service         |
| controller-rbac.yaml | Access control for CSI Controller service  |
| node-rbac.yaml       | Access control for CSI Node service        |
| config-map.yaml      | SPDK storage cluster configurations        |
| secret.yaml          | SPDK storage cluster access tokens         |

#### Deploy

1. Launch Minikube test cluster
  ```bash
  $ cd scripts
  $ sudo ./minikube.sh up

  # Wait for Kubernetes ready
  $ kubectl get pods --all-namespaces
  NAMESPACE     NAME                          READY   STATUS    RESTARTS   AGE
  kube-system   coredns-6955765f44-dlb88      1/1     Running   0          81s
  ......                                              ......
  kube-system   kube-apiserver-spdkcsi-dev    1/1     Running   0          67s
  ......                                              ......
  ```

2. Deploy SPDK-CSI services
  ```bash
  $ cd deploy/kubernetes
  $ ./deploy.sh

  # Check status
  $ kubectl get pods
  NAME                   READY   STATUS    RESTARTS   AGE
  spdkcsi-controller-0   3/3     Running   0          3m16s
  spdkcsi-node-lzvg5     2/2     Running   0          3m16s
  ```

### Teardown

1. Delete SPDK-CSI services
  ```bash
  $ cd deploy/kubernetes
  $ ./deploy.sh teardown
  ```

2. Teardown Kubernetes test cluster
  ```bash
  $ cd scripts
  $ sudo ./minikube.sh clean
  ```

## Communication and Contribution

Please join [SPDK community](https://spdk.io/community/) for communication and contribution.

Project progress is tracked in [Trello board](https://trello.com/b/nBujJzya/kubernetes-integration).
