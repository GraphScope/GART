.. GART documentation master file, created by
   sphinx-quickstart on Tue Aug 27 10:19:05 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :description: GART is a graph extension that includes an interface to an RDBMS and a dynamic graph store for online graph processing.
   :keywords: distributed-systems, distributed, graph-analytics, in-memory-storage, big-data-analytics, distributed-comp

GART: Graph Analysis on Relational Transactional Datasets

|Discussion| |License| |USENIX|

What is GART?
-----------------

Hybrid transactional/analytical processing (HTAP) is a new trend that processes OLTP and online analytical processing (OLAP) in the same system simultaneously.
Analogously, we term dynamic graph analysis processing workloads on transactional datasets as hybrid transactional/graph-analytical processing (**HTGAP**).
GART reuses transaction logs to replay graph data online for freshness instead of offline data migration for freshness and performance.

GART captures the data changes in different (relational) data sources (e.g., database systems, streaming systems) and converts them to graph data according to user-defined rules.

In detail, the workflow of GART can be broken into the following steps:

.. figure:: images/arch.png
   :alt: GART architecture




Features
^^^^^^^^

Transparent Data Model Conversion
~~~~~~~~~~~~~~~~~~~~~~

To adapt to rich workload flexibility, GART proposes transparent data model conversion by graph extraction interfaces, which define rules of relational-graph mapping.

We provide a sample definition file called `RGMappings`_.

Efficient Dynamic Graph Storage
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To ensure the performance of graph analytical processing (GAP), GART proposes an efficient dynamic graph storage with good locality that stems from key insights into HTGAP workloads, including:
1. an efficient and mutable compressed sparse row (CSR) representation to guarantee the locality of scanning edges;
2. a coarse-grained MVCC to reduce the temporal and spatial overhead of versioning;
3. a flexible property storage to efficiently run various GAP workloads.

Service-Oriented Deployment Model
~~~~~~~~~~~~~~~~~~~~~~

GART acts as a service to synchronize database changes to the graph store.
When pulled up as a service on its own, users can try out the full power of GART and different graph computation engines on the graph store.
At the same time, GART also provides a front-end, used as a database plug-in, currently supported as PostgreSQL extension.
Users can invoke GART's functions in the database client, such as RGMapping definitions, graph computation on the graph store, etc.

Use cases
^^^^^^^^^

.. panels::
   :header: text-center
   :container: container-lg pb-4
   :column: col-lg-4 col-md-4 col-sm-4 col-xs-12 p-2
   :body: text-center

   .. link-button:: #
      :type: url
      :text: Object manager
      :classes: btn-block stretched-link

   Put and get arbitrary objects using Vineyard, in a zero-copy way!

   ---

   .. link-button:: #
      :type: url
      :text: Cross-system sharing
      :classes: btn-block stretched-link

   Share large objects across computing systems.

   ---

   .. link-button:: #
      :type: url
      :text: Data orchestration
      :classes: btn-block stretched-link

   Vineyard coordinates the flow of objects and jobs on Kubernetes based on data-aware scheduling.

Get started now!
----------------

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: notes/getting-started
      :type: ref
      :text: User Guides
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Get started with Vineyard.

   ---

   .. link-button:: notes/cloud-native/deploy-kubernetes
      :type: ref
      :text: Deploy on Kubernetes
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Deploy Vineyard on Kubernetes and accelerate big-data analytical workflows on cloud-native
   infrastructures.

   ---

   .. link-button:: tutorials/tutorials
      :type: ref
      :text: Tutorials
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Explore use cases and tutorials where Vineyard can bring added value.

   ---

   .. link-button:: notes/developers
      :type: ref
      :text: Getting Involved
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Get involved and become part of the Vineyard community.

   ---

   .. link-button:: notes/developers/faq
      :type: ref
      :text: FAQ
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Frequently asked questions and discussions during the adoption of Vineyard.

Read the Paper
--------------

- Sijie Shen, Zihang Yao, Lin Shi, Lei Wang, Longbin Lai, Qian Tao, Li Su, Rong Chen, Wenyuan Yu, Haibo Chen, Binyu Zang, Jingren Zhou.
  `Bridging the Gap between Relational OLTP and Graph-based OLAP <https://www.usenix.org/system/files/atc23-shen.pdf>`_.
  USENIX Annual Technical Conference, Boston, MA, USA, July 2023. |USENIX|.

.. toctree::
   :maxdepth: 1
   :caption: User Guides
   :hidden:

   notes/getting-started.rst
   notes/architecture.rst
   notes/key-concepts.rst

.. toctree::
   :maxdepth: 1
   :caption: Cloud-Native
   :hidden:

   notes/cloud-native/deploy-kubernetes.rst
   notes/cloud-native/vineyard-operator.rst
   Command-line tool <notes/cloud-native/vineyardctl.md>

.. toctree::
   :maxdepth: 1
   :caption: Tutorials
   :hidden:

   tutorials/data-processing.rst
   tutorials/kubernetes.rst
   tutorials/extending.rst

.. toctree::
   :maxdepth: 1
   :caption: Developer Guides
   :hidden:

   notes/developers.rst
   notes/developers/faq.rst

.. _Mars: https://github.com/mars-project/mars
.. _GraphScope: https://github.com/alibaba/GraphScope
.. _RGMappings: https://github.com/GraphScope/GART/blob/main/vegito/test/schema/rgmapping-ldbc.yaml

.. |Discussion| image:: https://img.shields.io/badge/Discuss-Ask%20Questions-blue?logo=GitHub
   :target: https://github.com/GraphScope/GART/issues

.. |License| image:: https://img.shields.io/github/license/graphscope/gart
   :target: https://github.com/GraphScope/GART/blob/main/LICENSE

.. |USENIX| image:: https://img.shields.io/badge/USENIX-blue
   :target: https://www.usenix.org/conference/atc23/presentation/shen
