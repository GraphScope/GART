.. GART documentation master file, created by
   sphinx-quickstart on Tue Aug 27 10:19:05 2019.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. meta::
   :description: GART is a graph extension that includes an interface to an RDBMS and a dynamic graph store for online graph processing.
   :keywords: distributed-systems, distributed, graph-analytics, in-memory-storage, big-data-analytics, distributed-comp

GART: Graph Analysis on Relational Transactional Datasets
=================================================================

|Discussion| |License| |USENIX|

.. include:: notes/getting-started/intro-content

----------------

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: notes/getting-started/quick-start
      :type: ref
      :text: Getting Started
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Get started with GART.

   ---

   .. link-button:: notes/deployment/deploy-local
      :type: ref
      :text: Deployment
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Build environment and deploy GART.

   ---

   .. link-button:: tutorials/tutorials
      :type: ref
      :text: Tutorials
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Explore use cases and tutorials where GART can bring added value.

   ---

   .. link-button:: https://github.com/GraphScope/GART
      :type: url
      :text: Getting Involved
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Get involved and become part of the GART community.

Read the Paper
--------------

- Sijie Shen, Zihang Yao, Lin Shi, Lei Wang, Longbin Lai, Qian Tao, Li Su, Rong Chen, Wenyuan Yu, Haibo Chen, Binyu Zang, Jingren Zhou.
  `Bridging the Gap between Relational OLTP and Graph-based OLAP <https://www.usenix.org/system/files/atc23-shen.pdf>`_.
  USENIX Annual Technical Conference, Boston, MA, USA, July 2023. |USENIX|.

.. toctree::
   :maxdepth: 1
   :caption: Getting Started
   :hidden:

   notes/getting-started/introduction
   notes/getting-started/quick-start
   notes/getting-started/architecture
   notes/getting-started/key-concepts

.. toctree::
   :maxdepth: 1
   :caption: Deployment
   :hidden:

   notes/deployment/deploy-local
   notes/deployment/deploy-cloud

.. toctree::
   :maxdepth: 1
   :caption: Tutorials
   :hidden:

   notes/tutorials/data-source-config

.. |Discussion| image:: https://img.shields.io/badge/Discuss-Ask%20Questions-blue?logo=GitHub
   :target: https://github.com/GraphScope/GART/issues

.. |License| image:: https://img.shields.io/github/license/graphscope/gart
   :target: https://github.com/GraphScope/GART/blob/main/LICENSE

.. |USENIX| image:: https://img.shields.io/badge/USENIX-blue
   :target: https://www.usenix.org/conference/atc23/presentation/shen
