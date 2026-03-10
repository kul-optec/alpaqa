{% set machine = os.popen("uname -m").read().strip() %}

{% if machine in ["x86_64", "AMD64"] %}

    {% set ld_output = "" %}
    {% if os.path.exists("/lib64/ld-linux-x86-64.so.2") %}
        {% set ld_output = os.popen("/lib64/ld-linux-x86-64.so.2 --help 2>/dev/null").read() %}
    {% elif os.path.exists("/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2") %}
        {% set ld_output = os.popen("/lib/x86_64-linux-gnu/ld-linux-x86-64.so.2 --help 2>/dev/null").read() %}
    {% elif os.path.exists("/lib/ld-linux-x86-64.so.2") %}
        {% set ld_output = os.popen("/lib/ld-linux-x86-64.so.2 --help 2>/dev/null").read() %}
    {% endif %}
    
    {% if "x86-64-v4 (supported" in ld_output %}
include({{ os.path.join(profile_dir, "x86-64-v4.profile") }})
    {% elif "x86-64-v3 (supported" in ld_output %}
include({{ os.path.join(profile_dir, "x86-64-v3.profile") }})
    {% else %}
include({{ os.path.join(profile_dir, "generic.profile") }})
    {% endif %}

{% elif machine in ["aarch64", "arm64"] %}

    {% set cpuinfo = os.popen("cat /proc/cpuinfo 2>/dev/null | grep -m1 'CPU part'").read() %}
    {% if "0xd0b" in cpuinfo %}
include({{ os.path.join(profile_dir, "cortex-a76.profile") }})
    {% elif "0xd08" in cpuinfo %}
include({{ os.path.join(profile_dir, "cortex-a72.profile") }})
    {% elif "0xd03" in cpuinfo %}
include({{ os.path.join(profile_dir, "cortex-a53.profile") }})
    {% else %}
include({{ os.path.join(profile_dir, "generic.profile") }})
    {% endif %}

{% elif machine in ["armv7l", "armv8l"] %}

include({{ os.path.join(profile_dir, "generic.profile") }})

{% else %}

include({{ os.path.join(profile_dir, "generic.profile") }})

{% endif %}

[settings]
arch.microarch=native

[conf]
tools.build:cflags+=["-march=native"]
tools.build:cxxflags+=["-march=native"]
