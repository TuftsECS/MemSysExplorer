import os
import yaml
from colorama import Fore, Style


def get_deep(dictionary, key, default=None):
    if not isinstance(dictionary, dict):
        return default

    key = key.split(".", 1)
    if key[0] in dictionary:
        if len(key) > 1:
            return get_deep(dictionary[key[0]], key[1])
        else:
            return dictionary[key[0]]
    else:
        return default


def merge(a, b, ctx, name=""):
    """
    Merges dict B into dict A.
    """

    if a is None:
        return b
    if b is None:
        return a

    if isinstance(a, dict) and isinstance(b, dict):
        for key, value in b.items():
            if key in a:
                a[key] = merge(a[key], value, ctx, f"{name}.{key}")
            else:
                a[key] = value
        return a
    else:
        ctx.warning(name, "overwriting")
        return b


checks = []


class Config:
    def __init__(self, filename):
        with open(filename, "r") as file:
            self.c = yaml.load(file.read(), Loader=yaml.FullLoader)
    
    def get(self, key, default=None):
        return get_deep(self.c, key, default)

    def check(self):
        ctx = Context()
        for check in checks:
            if check(self, ctx) == False:
                return False, ctx.diagnostics
        return True, ctx.diagnostics


class Diagnostic:
    def __init__(self, type, location, message):
        self.type = type
        self.location = location
        self.message = message

    def __str__(self):
        return f"{self._color()}{self.type}{Style.RESET_ALL}: {self.location}: {self.message}"

    def _color(self):
        match self.type:
            case "error":
                return Fore.RED + Style.BRIGHT
            case "warning":
                return Fore.YELLOW + Style.BRIGHT


class Context:
    def __init__(self):
        self.diagnostics = []

    def error(self, location, message):
        self.diagnostics.append(Diagnostic("error", location, message))

    def warning(self, location, message):
        self.diagnostics.append(Diagnostic("warning", location, message))


def check_func(func):
    checks.append(func)
    return func


# BEGIN CHECKS


@check_func
def load_system_config(config, ctx):
    """
    If a system config path was specified, load it and replace the existing
    config section with its contents.
    """
    sys_config_path = config.get("system.sys_config_path")
    if sys_config_path is not None:
        print(f"loading {sys_config_path}")
        with open(sys_config_path, "r") as file:
            config.c["system"] = merge(yaml.load(file.read(), Loader=yaml.FullLoader), config.c["system"], ctx, "system")


@check_func
def load_tech_config(config, ctx):
    """
    If an array characterization config path was specified, load it and replace
    the existing config section with its contents.
    """
    array_characterization_config = config.get("tech.array_characterization_config")
    if array_characterization_config is not None:
        print(f"loading {array_characterization_config}")
        with open(array_characterization_config, "r") as file:
            config.c["tech"] = merge(yaml.load(file.read(), Loader=yaml.FullLoader), config.c["tech"], ctx, "tech")


@check_func
def check_basics(config, ctx):
    success = True

    script_dir = os.path.dirname(__file__)
    with open(os.path.join(script_dir, "config_options.yml"), "r") as file:
        options = yaml.load(file.read(), Loader=yaml.FullLoader)

    for option, props in options.items():
        value = config.get(option)

        # If the option is unset, continue. Throw an error if it was required.
        if value is None:
            if get_deep(props, "required", False):
                ctx.error(option, "option is required")
                success = False
            continue

        # Check the type of the option.
        type = get_deep(props, "type", "string")
        match type:
            case "string":
                if not isinstance(value, str):
                    ctx.error(option, "expected string")
                    success = False
            case "bool":
                if not isinstance(value, bool):
                    ctx.error(option, "expected boolean")
                    success = False
            case "int":
                if not isinstance(value, int):
                    ctx.error(option, "expected integer")
                    success = False
            case "float":
                if not isinstance(value, float) and not isinstance(value, int):
                    ctx.error(option, "expected float")
                    success = False
            case "path":
                if not isinstance(value, str):
                    ctx.error(option, "expected path")
                    success = False
                elif get_deep(props, "path-must-exist", bool):
                    if not os.path.exists(value):
                        ctx.error(option, "path must exist")
                        success = False
            case "select":
                values = get_deep(props, "values", [])
                if value not in values:
                    ctx.error(option, f"expected one of: {', '.join(values)}")
                    success = False
            case "list":
                if not isinstance(value, list):
                    ctx.error(options, "expected list")
                    success = False

    return success


def require(config, ctx, key):
    if config.get(key) == None:
        ctx.error(key, "option is required")
        return False
    return True


@check_func
def check_capacity(config, ctx):
    """
    system.Capacity is handled a little weirdly. It can be either a single
    dictionary describing a single capacity, or a list of capacities.
    """
    capacity = config.get("system.Capacity")
    if capacity is None:
        ctx.error("system.Capacity", "option is required")
        return False

    def check_single_capacity(capacity):
        if "Value" not in capacity:
            ctx.error("system.Capacity.Value", "option is required")
            return False
        if not isinstance(capacity["Value"], int):
            ctx.error("system.Capacity.Value", "expected integer")
            return False

        if "Unit" not in capacity:
            ctx.error("system.Capacity.Unit", "option is required")
            return False
        if capacity["Unit"] not in ("KB", "MB", "GB"):
            ctx.error("system.Capacity.Unit", "expected one of: KB, MB, GB")
            return False

        return True

    if isinstance(capacity, dict):
        return check_single_capacity(capacity)
    elif isinstance(capacity, list):
        success = True
        for c in capacity:
            if not check_single_capacity(c):
                success = False
        return success
    else:
        ctx.error("system.Capacity", "expected dict or list")
        return False


@check_func
def check_optimization_target(config, ctx):
    # TODO allow lists!
    ...


@check_func
def check_apps(config, ctx):
    """
    Checks specific to the apps section.
    """

    run = config.get("apps.run")
    profiler = config.get("apps.profiler")

    if run == "new":
        if not require(config, ctx, "apps.executable"):
            return False
        if profiler != "dynamorio":
            if not require(config, ctx, "apps.level"):
                return False
        if profiler == "sniper":
            if not require(config, ctx, "apps.config"):
                return False
        if profiler == "perf":
            if not require(config, ctx, "apps.arch"):
                return False
    else:
        if not require(config, ctx, "apps.patternconfig_path"):
            return False
        if profiler == "sniper":
            if not require(config, ctx, "apps.multithread"):
                return False

    if profiler == "dynamorio" and config.get("system.DesignTarget") == "cache":
        ctx.error("apps.profiler", "DynamoRIO cannot be used with a cache design target")
        return False


@check_func
def check_tech(config, ctx):
    """
    Checks specific to the tech section.
    """

    run = config.get("tech.run")

    if run == "new":
        if not require(config, ctx, "tech.array_characterization_config"):
            return False
    else:
        if not require(config, ctx, "tech.array_characterization_result_path"):
            return False

        # TODO: get result files from array_characterization_result_path (if dir)
        # TODO: check result_files for matching OptimizationTarget
        # TODO: check result_files for matching Capacity

    # TODO: default tech config
    # [T] WEIRD: default tech config if requirements not met
    # [T] array_characterization_config -> array_characterization_config path must exist (file or directory)
    # [T] array_characterization_config is file -> Associativity, MemoryCellInputFile required

    # [T] run == existing -> get result_files from array_characterization_result_path (if dir)
    # [T] run == existing -> check result_files for matching OptimizationTarget
    # [T] run == existing -> check result_files for matching Capacity
