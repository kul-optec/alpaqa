#/usr/bin/env zsh
_alpaqa_driver_completions() {
    local ret=1
    case $CURRENT in
        (2)
            _tilde_files && ret=0
        ;;

        (*)

            local method=""
            for element in "${words[@]}"; do
                if [[ "$element" == "method="* ]]; then
                    method="${element#method=}"
                fi
            done

            local -A suggestions
            for line in ${(f)"$("${words[1]}" --complete "$method" "$words[CURRENT]")"}; do
                suggestions+=("${line%%:*}" "${line#*:}")
            done

            if (( ${#suggestions} == 0 )); then
                # Nothing
            else
                pfx="$suggestions[_prefix]"; unset "suggestions[_prefix]"
                sfx="$suggestions[_suffix]"; unset "suggestions[_suffix]"
                compadd -P "$pfx" -S "$sfx" -d suggestions -k suggestions && ret=0
            fi
        ;;
    esac
    return ret
}

compdef _alpaqa_driver_completions alpaqa-driver
compdef _alpaqa_driver_completions alpaqa-driver_d
