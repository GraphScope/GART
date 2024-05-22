Run Graph Analysis
=========================

GART supports different approaches for graph analysis. The following sections describe how to run the different analysis methods.

Graph Analysis with GAE
------------------------

`GAE <https://graphscope.io/docs/analytical_engine/builtin_algorithms>`_ is a graph analysis framework that provides a set of graph algorithms for analyzing large-scale graphs. GAE is implemented in C++ and provides a Python interface for running graph algorithms on graphs stored in the GART format.

After running the GART server, you can run graph analysis using the GAE library by ``mpirun`` command.

.. code:: bash

    gart-env$ cd /workspace/gart/
    gart-env$ mpirun -n 1 ./apps/run_gart_app --read_epoch 0 --app_name sssp --sssp_source_label organisation --sssp_source_oid 0 --sssp_weight_name wa_work_from

Other Alternatives
------------------

Please refer to the `PostgreSQL Extension <pgx.html>`_. The PostgreSQL extension provides a set of graph algorithms that can be run on graphs stored in the GART format.
