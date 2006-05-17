function pgmsave(data, path)
    f = fopen(path, "wb");
    fprintf(f, "P5\n%d %d 255\n", size(data)(2), size(data)(1));
    for i = [1:rows(data)]
        fwrite(f, data(i,:));
    endfor
    fclose(f);
endfunction
