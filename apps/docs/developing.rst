5. Contributing
===============

This section provides guidelines for developers to extend the MemSysExplorer framework by adding custom profilers into the profiling pipeline. The core components required for extension include:

1. **FrontendInterface Abstract Class:**
   Acts as the execution core responsible for activating and managing profilers.

2. **PatternConfig Abstract Class:**
   Serves as a configuration bridge between raw profiler data and downstream modeling utilities.

3. **BaseMetadata Interface (optional):**
   Defines how system-level and runtime metadata should be captured during profiling.

All profiler modules, pattern configurations, and metadata handlers should reside in their respective subdirectories under `profilers/` to ensure consistency and maintainability.

4.1 Adding Custom Profilers
---------------------------

To add a new profiler, developers should subclass:

- `FrontendInterface` (for profiling behavior)
- `PatternConfig` (for visualization or modeling utilities)
- Optionally, `BaseMetadata` (to add system/hardware context)

Then, register your new components through the centralized registry system described below.

4.2 Dynamic Profiler Registration
---------------------------------

Starting with v1.1, MemSysExplorer uses a JSON-driven dynamic loader to register supported profilers. The file `built_profilers.json` controls which components are active.

**Example `built_profilers.json`:**

.. code-block:: json

   {
     "ncu": true,
     "perf": true,
     "nvbit": true,
     "dynamorio": false,
     "sniper": true
   }

Only profilers with a `true` flag will be registered dynamically at runtime.

**Unified Profiler Registry:**

Starting with v1.2, all module and class registration is controlled via a central dictionary:

.. code-block:: python

   PROFILER_REGISTRY = {
       "ncu": {
           "profiler": ("profilers.ncu.ncu_profilers", "NsightComputeProfilers"),
           "config": ("profilers.ncu.ncu_PatternConfig", "NsightComputeConfig"),
           "metadata": ("profilers.ncu.ncu_Metadata", "NsightMetadata")
       },
       "perf": {
           "profiler": ("profilers.perf.perf_profilers", "PerfProfilers"),
           "config": ("profilers.perf.perf_PatternConfig", "PerfConfig"),
           "metadata": ("profilers.perf.perf_Metadata", "PerfMetadata")
       },
       ...
   }

This centralized mapping allows for concise and scalable extension by defining all module-class paths in one place.

**Dynamic Registration Workflow:**

.. code-block:: python

   def register_profilers():
       for profiler, entries in PROFILER_REGISTRY.items():
           if built_profilers.get(profiler, False):
               module_name, class_name = entries["profiler"]
               module = importlib.import_module(module_name)
               profiler_class = getattr(module, class_name)
               FrontendInterface.register_profiler(profiler, profiler_class)

   def register_PatternConfig():
       for profiler, entries in PROFILER_REGISTRY.items():
           if built_profilers.get(profiler, False):
               module_name, class_name = entries["config"]
               config_class = getattr(importlib.import_module(module_name), class_name)
               PatternConfig.register_config(profiler, config_class)

   def register_MetadataClasses():
       for profiler, entries in PROFILER_REGISTRY.items():
           if built_profilers.get(profiler, False):
               module_name, class_name = entries["metadata"]
               metadata_class = getattr(importlib.import_module(module_name), class_name)
               FrontendInterface.register_metadata(profiler, metadata_class)

This architecture allows developers to enable or disable profiler components without modifying source logic beyond the registry dictionary.

4.3 Directory Structure
-----------------------

Profiler components should follow the modular structure below:

.. code-block:: text

   profilers/
   ├── FrontendInterface.py
   ├── PatternConfig.py
   ├── BaseMetadata.py
   ├── registry.py
   ├── built_profilers.json
   ├── ncu/
   │   ├── ncu_profilers.py
   │   ├── ncu_PatternConfig.py
   │   ├── ncu_Metadata.py
   ├── nvbit/
   │   ├── nvbit_profilers.py
   │   ├── nvbit_PatternConfig.py
   │   ├── nvbit_Metadata.py
   ├── perf/
   │   ├── perf_profilers.py
   │   ├── perf_PatternConfig.py
   │   ├── perf_Metadata.py
   ...

Each profiler module should include:

- `*_profilers.py` – Defines the `FrontendInterface` subclass.
- `*_PatternConfig.py` – Defines the pattern modeling behavior.
- `*_Metadata.py` – Extracts runtime system information.

4.4 Best Practices for Development
----------------------------------

- **Directory Isolation:** Keep each profiler's implementation self-contained within its subdirectory.
- **Fail Gracefully:** Ensure each module handles `FileNotFoundError`, `PermissionError`, or unsupported environments cleanly.
- **Avoid Hardcoding Paths:** Use `os.path` to determine paths dynamically from the script location.
- **Test Incrementally:** Run profiling and extraction in isolation before integrating.
- **Update the Registry:** Add your profiler to `PROFILER_REGISTRY` in `registry.py` to integrate with the dynamic loader.

4.5 Further Reading
-------------------

For a step-by-step guide to implementing your own `FrontendInterface`, visit:

.. toctree::
   :maxdepth: 2
   :caption: Developer Tutorials

   developing/fe_interface_tutorial

By following this registry-based structure, you ensure a clean separation of concerns, reduce boilerplate, and streamline extensibility for future memory technologies.

