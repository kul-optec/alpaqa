# bash completion for alpaqa-driver                        -*- shell-script -*-

# Bash only
[ -z "$BASH_VERSION" ] && return

_alpaqa_check_common_prefix() {
    local prefix="${1:0:1}"
    for element in "${@:2}"; do
        if [[ "${element}" != "${prefix}"* ]]; then
            return 1 # Not all elements have a common prefix
        fi
    done
    return 0 # All elements have a common prefix
}

_alpaqa_driver_completions() {

    # Don't consider an equal sign as a word break
    local cur words cword
    if ! _get_comp_words_by_ref -n = cur words cword &>/dev/null; then
        return 1 # bash-completion unavailable
    fi

    if [ ${cword} -eq 1 ] || [ ${cword} -eq 3 -a "${words[2]}" = ":" ]; then
        local cur="${words[cword]}"
        _filedir
    else
        local IFS=$'\n'
        # Find the method
        local method=""
        for element in "${words[@]}"; do
            if [[ "$element" == "method="* ]]; then
                method="${element#method=}"
            fi
        done
        # Get the suggestions
        local -a out=($("${words[0]}" --complete "${method}" "${cur}"))
        local -a suggestions
        local -A descriptions
        # Extract the prefix and suffix, and store as an associative array of
        # options and their description.
        local prefix=""
        local suffix=""
        for i in "${!out[@]}"; do
            if [[ "${out[$i]}" == "_prefix:"* ]]; then
                prefix="${out[$i]#_prefix:}"
                unset "${out[$i]}"
            elif [[ "${out[$i]}" == "_suffix:"* ]]; then
                suffix="${out[$i]#_suffix:}"
                unset "${out[$i]}"
            else
                suggestions+=("${out[$i]%%:*}")
                descriptions+=("${out[$i]%%:*}" "${out[$i]#*:}")
            fi
        done
        # Filter our candidates
        local -a arr=($(compgen -W "${suggestions[*]}" -- "${cur#${prefix}}"))
        # Correctly add our candidates to COMPREPLY
        if [ ${#arr[*]} -eq 0 ]; then
            # No candidates
            COMPREPLY=()
        elif [ ${#arr[*]} -eq 1 ]; then
            # The reply still considers everything after an equal sign a new
            # word
            if [ "${COMP_WORDS[$COMP_CWORD-1]}" == = ]; then
                COMPREPLY=("${arr[0]}$suffix")
            else
                COMPREPLY=("$prefix${arr[0]}$suffix")
            fi
        else
            COMPREPLY=()
            for k in "${arr[@]}"; do
                COMPREPLY+=("${descriptions[$k]}")
            done
            # If all abbreviated options share their first letters, we don't
            # want bash to replace the current argument by the first letters of
            # the abbreviation. Instead we want it to replace the argument by
            # the full (un-abbreviated) prefix followed by these letters.
            if _alpaqa_check_common_prefix "${COMPREPLY[@]}"; then
                COMPREPLY=()
                for k in "${arr[@]}"; do
                    COMPREPLY+=("$prefix${descriptions[$k]}")
                done
            fi
        fi
    fi

}

complete -F _alpaqa_driver_completions -o nospace alpaqa-driver
complete -F _alpaqa_driver_completions -o nospace alpaqa-driver_d
