#!/usr/bin/awk -f
# note: you can only use these generated strings with
# puts or to printf AFTER you have specified a "%s" format string. figure out some other way if you want to use the generated string AS the format string: you need to replace % with %% INSIDE strings.
{
    gsub(/\\/, "\\\\", $0)
    gsub(/"/, "\\\"", $0)
    printf "\"%s\\n\"\n", $0
}