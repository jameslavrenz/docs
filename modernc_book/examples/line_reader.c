/* Hosted example: bounded line input with overlong-record recovery. */
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

enum { MAX_LINE = 16 };
enum line_status { LINE_OK, LINE_END, LINE_TOO_LONG, LINE_INVALID, LINE_IO_ERROR };

/* On LINE_OK writes a terminated string and length.
   On other statuses neither output is changed. Overlong and invalid
   records are drained through newline or EOF before returning. */
static enum line_status read_line(FILE *stream,
                                  char output[MAX_LINE + 1],
                                  size_t *length)
{
    if (stream == nullptr || output == nullptr || length == nullptr)
        return LINE_INVALID;

    char candidate[MAX_LINE + 1] = {};
    size_t used = 0;
    bool overlong = false;
    bool malformed = false;
    int ch;

    while ((ch = fgetc(stream)) != EOF) {
        if (ch == '\n')
            break;
        if (ch == 0)
            malformed = true; /* NUL not part of this line grammar. */
        if (used == MAX_LINE)
            overlong = true;
        else if (!malformed && !overlong)
            candidate[used++] = (char)ch;
    }
    if (ferror(stream))
        return LINE_IO_ERROR;
    if (malformed)
        return LINE_INVALID;
    if (overlong)
        return LINE_TOO_LONG;
    if (ch == EOF && used == 0)
        return LINE_END;

    candidate[used] = '\0';
    memcpy(output, candidate, used + 1);
    *length = used;
    return LINE_OK;
}

int main(void)
{
    FILE *stream = tmpfile();
    if (stream == nullptr)
        return 1;
    const char input[] = "short\n"
                         "12345678901234567\n" /* 17, rejected */
                         "recovered\n"
                         "1234567890123456\n"  /* exactly 16 */
                         "final";               /* no newline */
    if (fwrite(input, 1, sizeof input - 1, stream) != sizeof input - 1 ||
        fseek(stream, 0, SEEK_SET) != 0) {
        fclose(stream);
        return 1;
    }

    char line[MAX_LINE + 1] = "unchanged";
    size_t length = 88;
    if (read_line(stream, line, &length) != LINE_OK ||
        strcmp(line, "short") != 0 || length != 5)
        goto fail;
    if (read_line(stream, line, &length) != LINE_TOO_LONG ||
        strcmp(line, "short") != 0 || length != 5)
        goto fail;
    if (read_line(stream, line, &length) != LINE_OK ||
        strcmp(line, "recovered") != 0 || length != 9)
        goto fail;
    if (read_line(stream, line, &length) != LINE_OK ||
        strcmp(line, "1234567890123456") != 0 || length != MAX_LINE)
        goto fail;
    if (read_line(stream, line, &length) != LINE_OK ||
        strcmp(line, "final") != 0 || length != 5)
        goto fail;
    if (read_line(stream, line, &length) != LINE_END)
        goto fail;
    if (fclose(stream) != 0)
        return 1;
    puts("line_reader: OK");
    return 0;
fail:
    fclose(stream);
    return 1;
}
