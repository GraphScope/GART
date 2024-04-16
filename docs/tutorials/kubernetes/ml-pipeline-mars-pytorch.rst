Machine learning with Vineyard on Kubernetes
--------------------------------------------

In this demonstration, we will build a fraudulent transaction classifier for
fraudulent transaction data. The process consists of the following
three main steps:

- :code:`prepare-data`: Utilize Vineyard to read and store data in a distributed manner.
- :code:`process-data`: Employ Mars to process the data across multiple nodes.
- :code:`train-data`: Use Pytorch to train the model on the distributed data.

We have three tables: user table, product table, and transaction table.
The user and product tables primarily contain user and product IDs, along with
their respective ``Feature`` vectors. Each record in the transaction table indicates
a user purchasing a product, with a ``Fraud`` label identifying whether the
transaction is fraudulent. Additional features related to these transactions are also
stored in the transaction table. You can find the three tables in the `dataset repo`_.
Follow the steps below to reproduce the demonstration. First, create a vineyard cluster
with 3 worker nodes.

.. code:: bash

    $ cd k8s && make install-vineyard

.. admonition:: Expected output
   :class: admonition-details

    .. code:: bash

        the kubeconfig path is /tmp/e2e-k8s.config
        Creating the kind cluster with local registry
        a16c878c5091c1e5c9eff0a1fca065665f47edb4c8c75408b3d33e22f0ec0d05
        Creating cluster "kind" ...
        ✓ Ensuring node image (kindest/node:v1.24.0) 🖼
        ✓ Preparing nodes 📦 📦 📦 📦
        ✓ Writing configuration 📜
        ✓ Starting control-plane 🕹️
        ✓ Installing CNI 🔌
        ✓ Installing StorageClass 💾
        ✓ Joining worker nodes 🚜
        Set kubectl context to "kind-kind"
        You can now use your cluster with:

        kubectl cluster-info --context kind-kind --kubeconfig /tmp/e2e-k8s.config

        Thanks for using kind! 😊
        configmap/local-registry-hosting created
        Installing cert-manager...
        namespace/cert-manager created
        customresourcedefinition.apiextensions.k8s.io/certificaterequests.cert-manager.io created
        customresourcedefinition.apiextensions.k8s.io/certificates.cert-manager.io created
        customresourcedefinition.apiextensions.k8s.io/challenges.acme.cert-manager.io created
        customresourcedefinition.apiextensions.k8s.io/clusterissuers.cert-manager.io created
        customresourcedefinition.apiextensions.k8s.io/issuers.cert-manager.io created
        customresourcedefinition.apiextensions.k8s.io/orders.acme.cert-manager.io created
        serviceaccount/cert-manager-cainjector created
        serviceaccount/cert-manager created
        serviceaccount/cert-manager-webhook created
        configmap/cert-manager-webhook created
        clusterrole.rbac.authorization.k8s.io/cert-manager-cainjector created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-issuers created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-clusterissuers created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-certificates created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-orders created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-challenges created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-ingress-shim created
        clusterrole.rbac.authorization.k8s.io/cert-manager-view created
        clusterrole.rbac.authorization.k8s.io/cert-manager-edit created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-approve:cert-manager-io created
        clusterrole.rbac.authorization.k8s.io/cert-manager-controller-certificatesigningrequests created
        clusterrole.rbac.authorization.k8s.io/cert-manager-webhook:subjectaccessreviews created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-cainjector created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-issuers created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-clusterissuers created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-certificates created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-orders created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-challenges created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-ingress-shim created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-approve:cert-manager-io created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-controller-certificatesigningrequests created
        clusterrolebinding.rbac.authorization.k8s.io/cert-manager-webhook:subjectaccessreviews created
        role.rbac.authorization.k8s.io/cert-manager-cainjector:leaderelection created
        role.rbac.authorization.k8s.io/cert-manager:leaderelection created
        role.rbac.authorization.k8s.io/cert-manager-webhook:dynamic-serving created
        rolebinding.rbac.authorization.k8s.io/cert-manager-cainjector:leaderelection created
        rolebinding.rbac.authorization.k8s.io/cert-manager:leaderelection created
        rolebinding.rbac.authorization.k8s.io/cert-manager-webhook:dynamic-serving created
        service/cert-manager created
        service/cert-manager-webhook created
        deployment.apps/cert-manager-cainjector created
        deployment.apps/cert-manager created
        deployment.apps/cert-manager-webhook created
        mutatingwebhookconfiguration.admissionregistration.k8s.io/cert-manager-webhook created
        validatingwebhookconfiguration.admissionregistration.k8s.io/cert-manager-webhook created
        pod/cert-manager-5dd59d9d9b-k9hkm condition met
        pod/cert-manager-cainjector-8696fc9f89-bmjzh condition met
        pod/cert-manager-webhook-7d4b5b8c56-fvmc2 condition met
        Cert-Manager ready.
        Installing vineyard-operator...
        The push refers to repository [localhost:5001/vineyard-operator]
        c3a672704524: Pushed
        b14a7037d2e7: Pushed
        8d7366c22fd8: Pushed
        latest: digest: sha256:ea06c833351f19c5db28163406c55e2108676c27fdafea7652500c55ce333b9d size: 946
        make[1]: Entering directory '/opt/caoye/v6d/k8s'
        go: creating new go.mod: module tmp
        /home/gsbot/go/bin/controller-gen rbac:roleName=manager-role crd:maxDescLen=0 webhook paths="./..." output:crd:artifacts:config=config/crd/bases
        cd config/manager && /usr/local/bin/kustomize edit set image controller=localhost:5001/vineyard-operator:latest
        /usr/local/bin/kustomize build config/default | kubectl apply -f -
        namespace/vineyard-system created
        customresourcedefinition.apiextensions.k8s.io/backups.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/globalobjects.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/localobjects.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/operations.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/recovers.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/sidecars.k8s.v6d.io created
        customresourcedefinition.apiextensions.k8s.io/vineyardds.k8s.v6d.io created
        serviceaccount/vineyard-manager created
        role.rbac.authorization.k8s.io/vineyard-leader-election-role created
        clusterrole.rbac.authorization.k8s.io/vineyard-manager-role created
        clusterrole.rbac.authorization.k8s.io/vineyard-metrics-reader created
        clusterrole.rbac.authorization.k8s.io/vineyard-proxy-role created
        clusterrole.rbac.authorization.k8s.io/vineyard-scheduler-plugin-role created
        rolebinding.rbac.authorization.k8s.io/vineyard-leader-election-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-kube-scheduler-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-manager-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-proxy-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-scheduler-plugin-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-scheduler-rolebinding created
        clusterrolebinding.rbac.authorization.k8s.io/vineyard-volume-scheduler-rolebinding created
        service/vineyard-controller-manager-metrics-service created
        service/vineyard-webhook-service created
        deployment.apps/vineyard-controller-manager created
        certificate.cert-manager.io/vineyard-serving-cert created
        issuer.cert-manager.io/vineyard-selfsigned-issuer created
        mutatingwebhookconfiguration.admissionregistration.k8s.io/vineyard-mutating-webhook-configuration created
        validatingwebhookconfiguration.admissionregistration.k8s.io/vineyard-validating-webhook-configuration created
        make[1]: Leaving directory '/opt/caoye/v6d/k8s'
        deployment.apps/vineyard-controller-manager condition met
        Vineyard-Operator Ready
        Installing vineyard cluster...
        vineyardd.k8s.v6d.io/vineyardd-sample created
        vineyardd.k8s.v6d.io/vineyardd-sample condition met
        Vineyard cluster Ready

Verify that all Vineyard pods are running.

.. code:: bash

    $ KUBECONFIG=/tmp/e2e-k8s.config kubectl get pod -n vineyard-system

.. admonition:: Expected output
   :class: admonition-details

    .. code:: bash

        NAME                                           READY   STATUS    RESTARTS   AGE
        etcd0                                          1/1     Running   0          68s
        etcd1                                          1/1     Running   0          68s
        etcd2                                          1/1     Running   0          68s
        vineyard-controller-manager-7f569b57c5-46tgq   2/2     Running   0          92s
        vineyardd-sample-6ffcb96cbc-gs2v9              1/1     Running   0          67s
        vineyardd-sample-6ffcb96cbc-n59gg              1/1     Running   0          67s
        vineyardd-sample-6ffcb96cbc-xwpzd              1/1     Running   0          67s

First, let's prepare the dataset and download it into the kind worker nodes as follows.

.. code:: bash

    $ worker=($(docker ps | grep kind-worker | awk -F ' ' '{print $1}'))
    $ for c in ${worker[@]}; do \
        docker exec $c sh -c "\
            mkdir -p /datasets; \
            cd /datasets/; \
            curl -OL https://raw.githubusercontent.com/GraphScope/gstest/master/vineyard-mars-showcase-dataset/{item,txn,user}.csv" \
      done

The `prepare-data` job primarily reads the datasets and distributes them across different
Vineyard nodes. For more information, please refer to the `prepare data code`_. To apply
the job, follow the steps below:

.. note::

    The `prepare-data` job needs to exec into the other pods. Therefore, you need to
    create a service account and bind it to the role under the namespace.
    Please make sure you can have permission to create the following role.

    .. code:: text

      - apiGroups: [""]
        resources: ["pods", "pods/log", "pods/exec"]
        verbs: ["get", "patch", "delete", "create", "watch", "list"]

.. code:: bash

    $ kubectl create ns vineyard-job && \
    kubectl apply -f showcase/vineyard-mars-pytorch/prepare-data/resources && \
    kubectl wait job -n vineyard-job -l app=prepare-data --for condition=complete --timeout=1200s

.. admonition:: Expected output
   :class: admonition-details

    .. code:: bash

        namespace/vineyard-job created
        clusterrolebinding.rbac.authorization.k8s.io/prepare-data-rolebinding created
        clusterrole.rbac.authorization.k8s.io/prepare-data-role created
        job.batch/prepare-data created
        serviceaccount/prepare-data created
        job.batch/prepare-data condition met

.. note::

    The `process-data` job needs to create a new namespace and deploy several kubernetes 
    resources in it. Please make sure you can have permission to create the following role.

    .. code:: text

        - apiGroups: [""]
          resources: ["pods", "pods/exec", "pods/log", "endpoints", "services"]
          verbs: ["get", "patch", "delete", "create", "watch", "list"]
        - apiGroups: [""]
          resources: ["namespaces"]
          verbs: ["get", "create", "delete"]
        - apiGroups: [""]
          resources: ["nodes"]
          verbs: ["get", "list"]
        - apiGroups: ["rbac.authorization.k8s.io"]
          resources: ["roles", "rolebindings"]
          verbs: ["patch", "get", "create", "delete"]
        - apiGroups: ["apps"]
          resources: ["deployments"]
          verbs: ["create"]

    Notice, the `process-data` job will require lots of permissions to deal 
    kubernetes resources, so please check the image of `process-data` job 
    if it is an official one.

The `prepare-data` job creates numerous dataframes in Vineyard. To combine these dataframes,
we use the appropriate join method in `mars`_. For more details, refer to the `process data
code`_. Apply the `process-data` job as follows:

.. code:: bash

    $ kubectl apply -f showcase/vineyard-mars-pytorch/process-data/resources && \
      kubectl wait job -n vineyard-job -l app=process-data --for condition=complete --timeout=1200s

Finally, apply the `train-data` job to obtain the fraudulent transaction classifier. You can
also view the `train data code`_.

.. code:: bash

    $ kubectl apply -f k8s/showcase/vineyard-mars-pytorch/train-data/resources && \
      kubectl wait pods -n vineyard-job -l app=train-data --for condition=Ready --timeout=1200s

If any of the above steps fail, please refer to the `mars showcase e2e test`_ for further guidance.


.. _mars: https://github.com/mars-project/mars
.. _mars showcase e2e test: https://github.com/v6d-io/v6d/blob/main/k8s/test/e2e/mars-examples/e2e.yaml
.. _dataset repo: https://github.com/GraphScope/gstest/tree/master/vineyard-mars-showcase-dataset
.. _prepare data code: https://github.com/v6d-io/v6d/blob/main/k8s/examples/vineyard-mars-pytorch/prepare-data/prepare-data.py
.. _process data code: https://github.com/v6d-io/v6d/blob/main/k8s/examples/vineyard-mars-pytorch/process-data/process-data.py
.. _train data code: https://github.com/v6d-io/v6d/blob/main/k8s/examples/vineyard-mars-pytorch/train-data/train-data.py
