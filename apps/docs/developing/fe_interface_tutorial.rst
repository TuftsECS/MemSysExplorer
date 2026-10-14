FrontendInterface Tutorial
==========================

The `FrontendInterface` is an abstract base class that defines a unified API for implementing profilers in MemSysExplorer. It ensures consistency across diverse backends (e.g., `ncu`, `perf`, `nvbit`, `sniper`) while supporting flexible profiling workflows.

.. important::

   **Do not modify the structure of the `FrontendInterface` class.**
   This class is foundational. Any changes to its method signatures or behavior can break compatibility with the system.

Instead of modifying the interface, always create new **subclasses** in your designated directory and register them through the framework.

Creating Custom Profilers
-------------------------

To implement and integrate your own profiler:

.. code-block:: bash

   cd profilers
   mkdir my_custom_profiler

Place your implementation inside this directory and follow the class structure outlined below.

Directory Structure Example:

.. code-block:: text

   profilers/
   └── my_custom_profiler/
       ├── custom_profilers.py
       ├── custom_PatternConfig.py
       └── custom_Metadata.py

Base Interface Overview
-----------------------

The `FrontendInterface` includes:

.. code-block:: python

   from abc import ABC, abstractmethod

   class FrontendInterface(ABC):
       registered_profilers = {}

       @abstractmethod
       def __init__(self, **kwargs):
           self.config = kwargs

       @abstractmethod
       def profiling(self, **kwargs):
           pass

       @abstractmethod
       def extract_metrics(self, **kwargs):
           pass

       @classmethod
       def required_profiling_args(cls):
           return []

       @classmethod
       def required_extract_args(cls, action):
           return []

       @classmethod
       def register_profiler(cls, name, profiler_class):
           cls.registered_profilers[name] = profiler_class

       @classmethod
       def create_profiler(cls, name, **kwargs):
           if name not in cls.registered_profilers:
               raise ValueError(f"Profiler '{name}' is not registered.")
           return cls.registered_profilers[name](**kwargs)

Implementing a Custom Profiler
------------------------------

**Step 1**: Define Your Profiler

.. code-block:: python

   from profilers.FrontendInterface import FrontendInterface

   class CustomProfiler(FrontendInterface):
       def __init__(self, **kwargs):
           super().__init__(**kwargs)

       def profiling(self, **kwargs):
           print("Running profiling logic...")

       def extract_metrics(self, **kwargs):
           return {"example_metric": 123}

       @classmethod
       def required_profiling_args(cls):
           return ["executable"]

       @classmethod
       def required_extract_args(cls, action):
           return ["report_file"] if action == "extract_metrics" else []

**Step 2**: Add Entry to `built_profilers.json`

.. code-block:: json

   {
     "ncu": true,
     "perf": true,
     "custom_profiler": true
   }

**Step 3**: Register in `PROFILER_REGISTRY`

Open `profilers/registry.py` and add your entry to the unified registry dictionary:

.. code-block:: python

   PROFILER_REGISTRY = {
       ...
       "custom_profiler": {
           "profiler": ("profilers.my_custom_profiler.custom_profilers", "CustomProfiler"),
           "config": ("profilers.my_custom_profiler.custom_PatternConfig", "CustomConfig"),
           "metadata": ("profilers.my_custom_profiler.custom_Metadata", "CustomMetadata")
       }
   }

This single registry entry is sufficient for all dynamic registration calls in the system.

**Step 4**: Register Automatically

When `register_profilers()`, `register_PatternConfig()`, and `register_MetadataClasses()` are called from the framework, your profiler will be registered if enabled in `built_profilers.json`.

.. important::

   When implementing a custom `PatternConfig`, make sure to **explicitly define the correct `unit` dictionary** that maps each metric field to its appropriate unit (e.g., `"read_freq": "bytes/s"` or `"total_reads": "count"`). This ensures consistent output and avoids misinterpretation in downstream memory analysis tools.

Using a Custom Profiler
-----------------------

Once registered, you can create and use your profiler via:

.. code-block:: python

   profiler = FrontendInterface.create_profiler("custom_profiler", executable="./my_app")
   profiler.profiling()
   metrics = profiler.extract_metrics(report_file="metrics.txt")
   print(metrics)

Retrieving Required Arguments
-----------------------------

Each subclass documents its required arguments via:

.. code-block:: python

   CustomProfiler.required_profiling_args()
   CustomProfiler.required_extract_args("extract_metrics")

Error Handling
--------------

1. **Unregistered Profiler:**

   .. code-block:: python

      FrontendInterface.create_profiler("unknown")
      # Raises: ValueError: Profiler 'unknown' is not registered.

2. **Missing Executable Path:**

   Ensure `executable` is passed to your profiler or defined in the config dictionary.

Best Practices
--------------

- Keep each profiler modular (in its own directory).
- Avoid side effects in imports (e.g., running code at import time).
- Use the pattern config class for formatting or visualization rules.
- Register your module centrally in `PROFILER_REGISTRY` for maintainability.
- Always set `unit` values in your `PatternConfig` to ensure proper output interpretation.

