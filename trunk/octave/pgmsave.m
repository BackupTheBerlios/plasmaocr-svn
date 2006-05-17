function pgmsave(data, path)
    f = fopen(path, "wb");
    fprintf(f, "P5\n%d %d 255\n", size(data)(2), size(data)(1));
    fwrite(f, data(:));
    fclose(f);
endfunction
