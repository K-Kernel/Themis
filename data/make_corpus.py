"""Generate the Themis CSV torture corpus.

Run from the project root:  python3 data/make_corpus.py
Writes raw bytes deliberately - do not edit these files in an editor,
it will silently normalise line endings and strip the BOM.
"""
import pathlib

HERE = pathlib.Path(__file__).parent

FILES = {
    # 1. delimiter inside a quoted field
    "quoted_delimiter.csv": b'a,"b,c",d\n1,"2,3",4\n',
    # 2. escaped quote: "" inside a quoted field means one literal "
    "escaped_quote.csv":    b'a,"he said ""hi""",b\n',
    # 3. record separator inside a quoted field
    "newline_in_quotes.csv": b'a,"line1\nline2",c\n',
    # 4. Windows line endings
    "crlf.csv":             b'a,b,c\r\n1,2,3\r\n',
    # 5. UTF-8 byte order mark, as Excel emits
    "bom.csv":              b'\xef\xbb\xbfa,b,c\n1,2,3\n',
    # 6. trailing delimiter -> real empty final field
    "trailing_delim.csv":   b'a,b,\n1,2,\n',
    # 7. ragged rows: one short, one long
    "ragged.csv":           b'a,b,c\n1,2\n4,5,6,7\n',
    # 8. no trailing newline (regression guard)
    "no_trailing_nl.csv":   b'a,b,c\n1,2,3',
    #9. final short row
    "final_short.csv": b'a,b,c\n1,2',
    #10. final long row
    "final_long.csv": b'a,b,c\n1,2,3,4',
    #11. single line
    "single_final.csv": b'a,b,c',
    #12. quote mid field
    "quote_mid_field.csv": b'ab\"cd,e',

    # --- record boundaries after a quoted field --------------------------
    #13. a later row that STARTS with a quoted field
    "quoted_first_field.csv":  b'name,age\n"Smith, John",42\n"Doe, Jane",37\n',
    #14. quoted LAST field across several rows
    "quoted_last_field.csv":   b'a,b,"c"\n1,2,"3"\n4,5,6\n',
    #15. quoted last field + CRLF
    "quoted_last_crlf.csv":    b'a,b,"c"\r\n1,2,"3"\r\n',

    # --- end of file in every state ---------------------------------------
    #16. closed quote at EOF, no newline
    "quoted_at_eof.csv":       b'a,b,"c"',
    #17. escaped field at EOF, no newline
    "escaped_at_eof.csv":      b'a,"x""y"',
    #18. escaped field + CRLF
    "escaped_crlf.csv":        b'a,"x""y"\r\n',
    #19. trailing delimiter at EOF, no newline
    "trailing_delim_eof.csv":  b'a,b,c\n1,2,',
    #20. unterminated quote: swallows to EOF, must not throw
    "unterminated_quote.csv":  b'a,b\n1,"oops\n3,4\n',

    # --- blank lines (policy: skipped, as pandas does) --------------------
    #21. file starts with a newline
    "leading_newline.csv":     b'\na,b\n1,2\n',
    #22. extra newline at end of file
    "trailing_blank_line.csv": b'a,b,c\n1,2,3\n\n',
    #23. blank line with CRLF
    "blank_line_crlf.csv":     b'a,b\r\n\r\n1,2\r\n',

    # --- error reporting --------------------------------------------------
    #24. short row: col must be the FIRST MISSING column
    "short_row_col.csv":       b'a,b,c,d\n1\n',
}


for file in HERE.iterdir():
    if file.name in FILES.keys():
        file.unlink()
        print(f"deleting : {file.name}")

for name, data in FILES.items():
    (HERE / name).write_bytes(data)
    print(f"{name:26} {len(data):4} bytes")
