# Comparison to ROS

* We want to move out of ROS approach to define microservices, where every component is a node and we create a massive data flow graph that gets hard to handle.
* The benefit of that approach is that it is easy to extend and to compose from existing components, however from our experience robotic systems that are past prototype stage do not have that requirement so the cost of it outweighs the benefit.
* We have a clear idea of what we want: determinism, easy inspectability and efficiency. This can be done with ROS but it is hard to do.
* We want a standard message format that is not custom but widely used and highly compatible.

*Example*:

For the Visual SLAM module instead of having one feature extraction, a feature cleaning, a prior estimation, a bundle adjustment and a mapping module, in hephaestus this all would be defined in one module that has images as input and map and pose as output.
However hephaestus supports easy use of thread pools and asynchronous containers to still easily parallelize workload.
Looking most of the SLAM code that is published, the ROS interface is usually just one node anyways. Hence hephaestus can be seen as facilitating multithreading by emposing clear architecture paradigms.
