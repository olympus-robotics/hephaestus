# Comparison to ROS

ROS is an excellent framework designed to maximize prototyping speed, flexibility, and code reuse, making it ideal for research labs and early-stage startups. However, building scalable, robust products often demands a different set of priorities: static configuration, determinism, high inspectability, optimized performance, and tailored resource usage.

Hephaestus is designed with these production requirements in mind. Compared to a typical ROS setup, Hephaestus offers:

* **Static and Deterministic by Design:** Configuration is handled statically, eliminating runtime parameter changes and ensuring predictable system behavior crucial for reliable deployment.
* **Performance Optimization:**
    * Allows the use of custom data types and serialization methods, enabling fine-tuning for specific performance needs and reducing overhead.
    * The `conduit` module facilitates grouping related components into fewer, larger processes. This minimizes Inter-Process Communication (IPC) overhead and potential errors while maintaining modularity and encapsulation.
* **Built-in Inspectability:** Provides an integrated profiling and metrics system for automatic runtime performance analysis, simplifying debugging and optimization.
* **Strong Platform/Algorithm Decoupling:** Enforces a clear separation, allowing platform details (like communication or scheduling) to evolve independently from core algorithms. 
* **Modern C++ and Code Quality:** Leverages modern C++ standards and integrates with static analysis (clang-tidy) and formatting (clang-format) tools to enforce high code quality and maintainability.

