oldpwd="$PWD"
cd "$( dirname "${BASH_SOURCE[0]:-${(%):-%x}}" )"/../..

triple=x86_64-bionic-linux-gnu
tools_dir="$PWD/toolchains"
pfx="$tools_dir/x-tools/$triple"

export PATH="$PWD/staging/bin:$PATH"
export LD_LIBRARY_PATH="$pfx/$triple/lib64:$PWD/staging/lib:$LD_LIBRARY_PATH"
export LD_PRELOAD="$(realpath "$pfx/$triple/lib64/libstdc++.so"):$LD_PRELOAD"
export LD_PRELOAD="$(realpath "$pfx/$triple/lib64/libgfortran.so"):$LD_PRELOAD"
export LD_PRELOAD="$(realpath "$pfx/$triple/lib64/libgcc_s.so.1"):$LD_PRELOAD"

BASH_COMPLETION_PATH="$PWD/staging/share/bash-completion/completions/alpaqa-driver"
ZSH_COMPLETION_PATH="$PWD/staging/share/zsh/site-functions/alpaqa-driver.autocomplete.zsh"
CURRENT_SHELL="$(ps -p $$ -o comm=)"
case "$CURRENT_SHELL" in
    bash) source "$BASH_COMPLETION_PATH" ;;
    zsh) source "$ZSH_COMPLETION_PATH" ;;
    *) echo "Unsupported shell: $CURRENT_SHELL. Completion not loaded." ;;
esac

cd "$oldpwd"
unset oldpwd
