import json
import urllib.request
import yaml
from pathlib import Path
from dataclasses import dataclass

CUDA_URL: str = "https://developer.download.nvidia.com/compute/cuda/redist"


@dataclass
class Config:
    target_arch: str
    host_arch: str
    cuda_version: str
    host_packages: tuple[str, ...] = (
        "cuda_cuobjdump",
        "cuda_cuxxfilt",
        "cuda_gdb",
        "cuda_nvcc",
        "cuda_nvdisasm",
        "cuda_nvprune",
    )
    target_packages: tuple[str, ...] = (
        "cuda_cccl",
        "cuda_cudart",
        "cuda_compat",
        "cuda_cupti",
        "cuda_nvml_dev",
        "cuda_nvrtc",
        "cuda_nvtx",
        "cuda_opencl",
        "cuda_profiler_api",
        "cuda_sanitizer_api",
        "libcublas",
        "libcudla",
        "libcufft",
        "libcufile",
        "libcurand",
        "libcusolver",
        "libcusparse",
        "libnpp",
        "libnvidia_nscq",
        "libnvjitlink",
        "libnvjpeg",
    )


def conan_arch_name(a: str) -> str:
    return {"aarch64": "armv8"}.get(a, a)


def fetch_json(url: str) -> dict:
    print(f"Fetching {url}...")
    with urllib.request.urlopen(url) as response:
        return json.load(response)


def generate_conandata(config: Config, cuda_data: dict, conandata: dict):
    sources = (
        conandata.setdefault("sources", {})
        .setdefault(config.cuda_version, {})
        .setdefault(config.host_arch, {})
    )

    for pkg in config.host_packages:
        try:
            pkg_data = cuda_data[pkg][f"linux-{config.host_arch}"]
            sources.setdefault("host", {})[pkg] = {
                "url": f"{CUDA_URL}/{pkg_data['relative_path']}",
                "sha256": pkg_data["sha256"],
            }
        except KeyError as e:
            print(f"Skipping {pkg}:", e)

    for pkg in config.target_packages:
        try:
            pkg_data = cuda_data[pkg][f"linux-{config.target_arch}"]
            sources.setdefault(conan_arch_name(config.target_arch), {})[pkg] = {
                "url": f"{CUDA_URL}/{pkg_data['relative_path']}",
                "sha256": pkg_data["sha256"],
            }
        except KeyError as e:
            print(f"Skipping {pkg}:", e)


def main():
    script_dir = Path(__file__).resolve().parent

    cuda_versions = ["12.8.0", "12.6.3", "12.4.1", "12.2.2"]
    archs = ["x86_64", "aarch64"]

    conandata = {}
    for cuda_version in cuda_versions:
        cuda_data = fetch_json(f"{CUDA_URL}/redistrib_{cuda_version}.json")

        for arch in archs:
            config = Config(
                target_arch=arch,
                host_arch="x86_64",
                cuda_version=cuda_version,
            )
            generate_conandata(config, cuda_data, conandata)

    conandata_file = script_dir / "binary" / "conandata.yml"
    with open(conandata_file, "w") as f:
        yaml.dump(conandata, f, default_flow_style=False, sort_keys=False)
    print(f"Conandata written to {conandata_file}")

    config_file = script_dir / "config.yml"
    folders = {"versions": {v: {"folder": "binary"} for v in cuda_versions}}
    with open(config_file, "w") as f:
        yaml.dump(folders, f, default_flow_style=False, sort_keys=False)
    print(f"Config written to {config_file}")


if __name__ == "__main__":
    main()
