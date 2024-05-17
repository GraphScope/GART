Environment Setup
====================

This section describes how to deploy GART locally.

Requirements
------------

- `glog`_
- `gflags`_
- `etcd-cpp-apiv3`_
- `TBB`_
- `librdkafka`_
- `Vineyard`_
- `Apache Kafka`_
- `Debezium`_ (preferred) or `Maxwell`_


Install Dependencies
--------------------

To simplify installing dependencies, we provide a script (`scripts/install-deps.sh`_) for installing GART's dependency environment.

.. code-block:: bash
    :linenos:

    $ git clone https://github.com/GraphScope/GART.git gart
    $ cd gart
    $ ./scripts/install-deps.sh /path/to/your/installation # install dependencies

Compile and Install GART
------------------------

.. code-block:: bash
    :linenos:

    $ mkdir -p build; cd build
    $ cmake .. -DCMAKE_BUILD_TYPE=Release
    $ make -j
    $ sudo make install

.. _glog: https://github.com/google/glog
.. _gflags: https://github.com/gflags/gflags
.. _etcd-cpp-apiv3: https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3
.. _TBB: https://github.com/oneapi-src/oneTBB
.. _librdkafka: https://github.com/confluentinc/librdkafka
.. _Vineyard: https://github.com/v6d-io/v6d
.. _Apache Kafka: https://kafka.apache.org/quickstart
.. _Debezium: https://github.com/debezium/debezium
.. _Maxwell: https://github.com/zendesk/maxwell
.. _scripts/install-deps.sh: https://github.com/GraphScope/GART/blob/main/scripts/install-deps.sh
