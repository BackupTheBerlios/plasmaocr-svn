# Show a graymap (0..255 uint8 or [0,256) double array) through gimv.
# (I don't like standard viewers)

function gm(A)
    pgm_name = sprintf("%s.pgm", tmpnam());
    pgmsave(A, pgm_name);
    show = sprintf ("gimv \"%s\"", pgm_name);
    rm = sprintf ("rm -f \"%s\"", pgm_name);
    system (sprintf ("( %s && %s ) 2>&1 >/dev/null &",
                      show, rm));  
endfunction
