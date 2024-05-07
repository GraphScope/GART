Data Source Configuration
==============================

.. toctree::
   :maxdepth: 1
   :caption: TOC
   :hidden:

   data-source-config/postgresql
   data-source-config/mysql
   data-source-config/storage


GART supports different types of data sources for data model transformation to provide real-time graph analytic processing. Because of the need to keep track of data updates, some permissions need to be configured on the data sources.

In this section, we will introduce the configuration of the data sources supported by GART: PostgreSQL and MySQL. At the same time, GART also supports arbitrary data streams, which only require the user to manually convert the data updates to the specified format.

.. panels::
   :header: text-center
   :column: col-lg-12 p-2

   .. link-button:: data-source-config/postgresql
      :type: ref
      :text: PostgreSQL
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   PostgreSQL configuration guide.

   ---

   .. link-button:: data-source-config/mysql
      :type: ref
      :text: MySQL
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   MySQL configuration guide.

   ---

   .. link-button:: data-source-config/storage
      :type: ref
      :text: GART Storage
      :classes: btn-block stretched-link
   ^^^^^^^^^^^^
   Guidelines for the use of GART's dynamic graph storage.
