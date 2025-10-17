.. _heph.conduit:

#######
Conduit
#######

In order to write complex applications within the context of robotics, a mechanism which allows us to connect different components between different steps is to be developed. This shall cover handling and forwarding of sensor data toward closing feedback loops. While a pub-sub model using topics is the de-facto goto solution in most robotic platforms, it also introduces a big source of nondeterminism and challenges regarding testability as well as programmability. In addition, a big source of complexity comes from handling different timing of different inputs as well as rate limiting the publishing of data.

.. todo:: Related Work

 - Relation to ROS
 - Relation to Zenoh
 - Relation to Zenoh Flow

*********
Objective
*********


The primary goal of this project is to provide a lean robotics platform to provide a modular and testable framework for robotics development.

The problem domain can be summarized with building a pipeline through which data can flow asynchronously. This includes abstractions to define, create and working these graphs.

.. _heph.conduit.concepts:

********
Concepts
********

The core idea is centered around a defining a step requiring a given set of inputs and producing an set of outputs. Connecting different steps together is the cornerstone of `Conduit` forming a graph of connecting nodes. As a result of this, we define the following requirements from which we derive these core concepts:

- **Inputs**: An input is what defines *when* a step function should get scheduled for execution.
- **Outputs**: The output is publishing the result of the execution (the *what*).
- **Stepper**: Steppers define *how* the inputs get consumed and the outputs produced.
- **Graph Description**: The *why* is handled by describing the connections and hierarchy for our platform.


Putting these concepts together will result in a **Conduit Graph** describing the hierarchical structure of **Nodes**. This graph can then be used to derive a directed execution graph which is getting executed based on the defined policies and connections.

Following is the definition of these core concepts and their API. Implementations of these concepts can be found in :ref:`heph.conduit.usage`.

.. note:: While the primary focus of conduit is to provide an abstraction for building a computational graph, Inputs and Outputs can be used outside of graphs as well as building blocks to interface with external sources.

.. _heph.conduit.concepts.inputs:

Inputs
======

As an Input to a step function is a very vague statement, the most basic requirement is the ability to signal a result. As such, we define the following interface:

.. literalinclude:: ../modules/conduit/include/hephaestus/conduit/basic_input.h
   :caption: :cpp:class:`heph::conduit::BasicInput`
   :language: cpp
   :start-after: // APIDOC: begin
   :end-before: // APIDOC: end

This definition allows to define arbitrary implementations on how/when something can be marked as ready. Implementations of this interface can specify additional APIs on top, and provide a particular implementation for the trigger. By using the generic concept of a :ref:`heph.concurrency.concepts.sender`, we allow for maximum flexibility. This allows to implement `Inputs` for various use cases, including the retrieval of sensor data towards providing interoperability with Zenoh.

.. _heph.conduit.concepts.inputs.basic:

Basic Inputs
------------

Conduit provides an implementation for the following Basic Inputs:

- :ref:`Periodic <heph.conduit.usage.inputs.periodic>`
- :ref:`Conditional <heph.conduit.usage.inputs.conditional>`
- :ref:`Generator <heph.conduit.usage.inputs.generator>`
.. - :ref:`ZenohSubscriber <heph.conduit.usage.inputs.zenoh_subscriber>`
.. - :ref:`ActionService <heph.conduit.usage.inputs.action_service>`

All of them form a root of the execution graph and their main purpose is to provide abstractions to interact with the external components. They are not connectable to any outputs. Their produced value (if any), can only be consumed directly from the APIs the specific implementation provides. All of them can be used outside conduit nodes without further ado and provide re-usable components for other tasks.

.. _heph.conduit.concepts.inputs.typed:

Typed Inputs
--------------

In addition to Basic Inputs we need Inputs that can be used to model the flow of data through the graph. These inputs require policies that describe their semantics with respect to how data is stored between each invocation of :cpp:func:`heph::conduit::BasicInput::trigger` and how cancelation is modelled. Thes requirements stems from the fact that each step has different requirements on when the input is triggered as well as other constraints such as defining deadlines.

This can be summarized by providing policies for :ref:`heph.conduit.concepts.inputs.typed.storage` and :ref:`heph.conduit.concepts.inputs.typed.triggers`.

As they produce values, they are called typed **Input** (:cpp:class:`full API Reference <heph::conduit::Input>`).

.. _heph.conduit.concepts.inputs.typed.storage:

Value Storage
^^^^^^^^^^^^^

why???

.. _heph.conduit.concepts.inputs.typed.triggers:

Triggers
^^^^^^^^

.. _heph.conduit.concepts.outputs:

Outputs
=======

.. _heph.conduit.concepts.stepper:

Stepper
=======


.. _heph.conduit.concepts.graph_description:

Graph Description
=================

.. _heph.conduit.usage:

*****
Usage
*****

.. _heph.conduit.usage.inputs:

Inputs
======


.. _heph.conduit.usage.inputs.periodic:

Periodic
--------

A :cpp:class:`Periodic <heph::conduit::Periodic>` Input is serving the purpose of allowing a stepper to be executed in a fixed periodic time duration. The completion of a periodic aims to keep the elapsed time between each periodic invocation constant to avoid drift. Avoiding jitter is not within the scope of the periodic trigger and is the responsibility of the scheduler.

:cpp:func:`heph::conduit::Periodic::setPeriodDuration` is setting the period duration and needs to be called before :cpp:func:`trigger <heph::conduit::BasicInput::trigger>` gets called.

Use Cases:

- External events such as keyboard or gamepad which require polling
- Implement spinning loops with a fixed period time

.. note:: Timing isn't always exact, ``heph::conduit::ClockT`` is based on ``std::chrono::system_clock``.

Example:

.. literalinclude:: ../modules/conduit/examples/periodic_spinner.cpp
   :caption: Periodic spinner example
   :language: cpp
   :start-after: // APIDOC: begin
   :end-before: // APIDOC: end

Example execution:

.. code-block:: none

   $ bazel run //modules/conduit:periodic_spinner
  INFO: Invocation ID: 50722712-613b-4195-8737-975db4fa5abf
  INFO: Analyzed target //modules/conduit:periodic_spinner (0 packages loaded, 0 targets configured).
  INFO: Found 1 target...
  Target //modules/conduit:periodic_spinner up-to-date:
    bazel-bin/modules/conduit/periodic_spinner
  INFO: Elapsed time: 0.259s, Critical Path: 0.01s
  INFO: 1 process: 5 action cache hit, 1 internal.
  INFO: Build completed successfully, 1 total action
  INFO: Running command line: bazel-bin/modules/conduit/periodic_spinner
  Starting Spinner, exit by pressing ctrl+c
  Time elapsed since last spin: 00.00s
  Time elapsed since last spin: 09.99s
  Time elapsed since last spin: 10.00s
  Time elapsed since last spin: 09.99s
  Time elapsed since last spin: 09.99s
  Time elapsed since last spin: 10.00s
  Time elapsed since last spin: 10.00s
  Time elapsed since last spin: 09.99s
  Time elapsed since last spin: 10.00s
  Time elapsed since last spin: 10.00s
  ^C$


.. _heph.conduit.usage.inputs.conditional:

Conditional
-----------

The :cpp:class:`Condtional <heph::conduit::Conditional>` can be used to drive conditional execution. The :cpp:func:`trigger <heph::conduit::BasicInput::trigger>` function will only complete if the conditional is enabled. Otherwise it will block:

- :cpp:func:`heph::conduit::Conditional::enable` will enable the conditional
- :cpp:func:`heph::conduit::Conditional::disable` disables it.

The primary use cases is to maintain a single execution graph while having various features configurable through external means such as zenoh action calls.

.. _heph.conduit.usage.inputs.generator:

Generator
---------

In order to inject values into the computational graph from external resources, you can use :cpp:class:`Generator <heph::conduit::Generator>`. :cpp:func:`trigger <heph::conduit::BasicInput::trigger>` will invoke the supplied generator function (see :cpp:func:`heph::conduit::Generator::setGenerator`) to supply the trigger. If the generator function does not return a sender, it will be wrapped to return a sender which is completing immediately, otherwise the trigger will return ones the returned sender completes.

Use Cases:

- Reading sensor data without polling
- Providing input data for Testing

.. _heph.conduit.usage.inputs.zenoh_subscriber:

ZenohSubscriber
---------------

.. todo:: TODO

.. - `ZenohSubscriber`
    - `trigger` will complete whenever data is published on the subscribed topic.
    - Requires `setTopic` to be called during the connection phase.
    - Use Case: Zenoh interoperability

.. _heph.conduit.usage.inputs.action_service:

ActionService
-------------

.. todo:: TODO

.. - `ActionService`
    - `trigger` will complete whenever a service request was made.
    - Requires `setService` to be called during the connection phase.
    - Use Case: Zenoh interoperability: React on Zenoh Action Service requests. This is similar to PollingActionService.

Outputs
=======

.. _heph.conduit.telemetry:

*********
Telemetry
*********

.. _heph.conduit.examples:

********
Examples
********

