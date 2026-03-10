{% set home = os.getenv("HOME") %}
{% set mkl_candidates = [
    os.getenv("MKLROOT"),
    os.path.join(home, "intel/oneapi/mkl/latest") if home else None,
    "/opt/intel/oneapi/mkl/latest",
] %}
{% set vtune_candidates = [
    os.getenv("VTUNE_PROFILER_DIR"),
    os.path.join(home, "intel/oneapi/vtune/latest") if home else None,
    "/opt/intel/oneapi/vtune/latest",
] %}
{% set paths = namespace(mkl_root=None, vtune_dir=None) %}
{% for candidate in mkl_candidates %}
    {% if candidate and not paths.mkl_root %}
        {% if os.path.isdir(candidate) %}
            {% set paths.mkl_root = candidate %}
        {% endif %}
    {% endif %}
{% endfor %}
{% for candidate in vtune_candidates %}
    {% if candidate and not paths.vtune_dir %}
        {% if os.path.isdir(candidate) %}
            {% set paths.vtune_dir = candidate %}
        {% endif %}
    {% endif %}
{% endfor %}

[buildenv]
{% if paths.mkl_root %}
MKLROOT={{ paths.mkl_root }}
{% endif %}
{% if paths.vtune_dir %}
VTUNE_PROFILER_DIR={{ paths.vtune_dir }}
{% endif %}

[conf]
{% if paths.mkl_root %}
tools.cmake.cmaketoolchain:extra_variables*={"MKL_ROOT": "{{ paths.mkl_root }}"}
{% endif %}
