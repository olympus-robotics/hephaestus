.. _heph.documentation:

#####################
Writing Documentation
#####################

Hephaestus is using sphinx+breathe to generate its documentation. The documentation generation is done via bazel.

In order to build the documentation invoke the following command:

.. code-block:: bash

   bazel build docs

This will generate the full documentation but might take some time due to the C++ API doxygen comment extraction.

To allow for faster iteration, there is an additional target which excludes the API docs:

.. code-block:: bash

   bazel build docs-fast


To get a live preview of the documentation use the following:

.. code-block:: bash

   bazel run docs.serve      # Get live preview of everything
   bazel run docs-fast.serve # Get live preview without the C++ API reference

********************
Module Documentation
********************

We are using `reStructuredText <rst>`_. The documentation of a module is done via ``doc/<module>.rst``. Make sure that this document is referenced in the respective toctree of ``doc/index.rst``.

In addition, you can add C++ API reference documentation. This is done with the help of `Breathe <breathe>`_. To include your library documentation, add it it to the  ``heph_cc_api_doc`` call in the top level ``BUILD`` file. To reference an entitiy of the reference documentation, use `Breathe Domains <breathe_domain>`_.

.. _rst: https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html
.. _breathe: https://breathe.readthedocs.io/en/latest
.. _breathe_domain: https://breathe.readthedocs.io/en/latest/domains.html

*********************
Documentation tooling
*********************

The tooling is provided by ``pypi`` managed by bazel. In order to add new tooling related
dependencies, you need to add your pip package to ``requirements.txt`` and freeze these packages:

.. code-block:: bash

   python3 -m venv venv && . venv/bin/activate
   pip install -r requirements.txt && python3 -m pip freeze > requirements.lock  && deactivate

After having executed these steps, the newly updated packages are used in the next build invocation.
