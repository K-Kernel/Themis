"""Generate Themis CSV test.

Run from the project root:  python3 data/make_corpus.py
Writes raw bytes deliberately - do not edit these files in an editor,
it will silently normalise line endings and strip the BOM.
"""

import pathlib

DATA_DIR = pathlib.Path(__file__).parent.parent
OUT_DIR = DATA_DIR / "corpus"

FILES = {
    # 1. delimiter inside a quoted field
    "quoted_delimiter.csv": b'a,"b,c",d\n1,"2,3",4\n',
    # 2. escaped quote: "" inside a quoted field means one literal "
    "escaped_quote.csv": b'a,"he said ""hi""",b\n',
    # 3. record separator inside a quoted field
    "newline_in_quotes.csv": b'a,"line1\nline2",c\n',
    # 4. Windows line endings
    "crlf.csv": b"a,b,c\r\n1,2,3\r\n",
    # 5. UTF-8 byte order mark, as Excel emits
    "bom.csv": b"\xef\xbb\xbfa,b,c\n1,2,3\n",
    # 6. trailing delimiter -> real empty final field
    "trailing_delim.csv": b"a,b,\n1,2,\n",
    # 7. ragged rows: one short, one long
    "ragged.csv": b"a,b,c\n1,2\n4,5,6,7\n",
    # 8. no trailing newline (regression guard)
    "no_trailing_nl.csv": b"a,b,c\n1,2,3",
    # 9. final short row
    "final_short.csv": b"a,b,c\n1,2",
    # 10. final long row
    "final_long.csv": b"a,b,c\n1,2,3,4",
    # 11. single line
    "single_final.csv": b"a,b,c",
    # 12. quote mid field
    "quote_mid_field.csv": b'ab"cd,e',
    # --- record boundaries after a quoted field --------------------------
    # 13. a later row that STARTS with a quoted field
    "quoted_first_field.csv": b'name,age\n"Smith, John",42\n"Doe, Jane",37\n',
    # 14. quoted LAST field across several rows
    "quoted_last_field.csv": b'a,b,"c"\n1,2,"3"\n4,5,6\n',
    # 15. quoted last field + CRLF
    "quoted_last_crlf.csv": b'a,b,"c"\r\n1,2,"3"\r\n',
    # --- end of file in every state ---------------------------------------
    # 16. closed quote at EOF, no newline
    "quoted_at_eof.csv": b'a,b,"c"',
    # 17. escaped field at EOF, no newline
    "escaped_at_eof.csv": b'a,"x""y"',
    # 18. escaped field + CRLF
    "escaped_crlf.csv": b'a,"x""y"\r\n',
    # 19. trailing delimiter at EOF, no newline
    "trailing_delim_eof.csv": b"a,b,c\n1,2,",
    # 20. unterminated quote: swallows to EOF, must not throw
    "unterminated_quote.csv": b'a,b\n1,"oops\n3,4\n',
    # --- blank lines (policy: skipped, as pandas does) --------------------
    # 21. file starts with a newline
    "leading_newline.csv": b"\na,b\n1,2\n",
    # 22. extra newline at end of file
    "trailing_blank_line.csv": b"a,b,c\n1,2,3\n\n",
    # 23. blank line with CRLF
    "blank_line_crlf.csv": b"a,b\r\n\r\n1,2\r\n",
    # --- error reporting --------------------------------------------------
    # 24. short row: col must be the FIRST MISSING column
    "short_row_col.csv": b"a,b,c,d\n1\n",
    # --- Final 4 cases -----------------------------------
    # 25. quoted as final byte
    "quote_final_byte.csv": b'a,"',
    # 26. Escaped quote inside an unclosed field
    "escaped_quote_unclosed.csv": b'"""a',
    # 27. Character after closing quote
    "character_after_closing_quote.csv": b'"ab"x,c',
    # 28.\r (carriage return)
    "cr.csv": b"a,b\rc,d",
    # 29. carraige return at the start
    "cr_at_the_start.csv": b"a,\rc,d",
    # 30. Carriage return after closing quote
    "cr_after_closing_quote.csv": b'a,"b"\rc,d',
    # 31. Data after quotes finished
    "stray_after_escape.csv": b'"a""b"x,c\n',
    # 32. Carriage return after clos
    "stray_then_newline.csv": b'x,"a"b\nc,d\n',
    # ---  Header --------------------------------------------------
    "header.csv": b"id,name\n1,Anna\n",
    "header_spaces.csv": b"id, name\n1,Ann\n",
    "header_short_row.csv": b"a,b\n1\n",
    "header_quoted.csv": b'"x,y",z\n1,2\n',
    "header_only.csv": b"a,b\n",
    "header_duplicate.csv": b"a,a\n1,2",
    # ---  Infer --------------------------------------------------
    "integer.csv": b"n\n1\n-2\n30\n",
    "double.csv": b"x\n1.5\n-2\n",
    "string.csv": b"v\n12abc\n",
    "nulls.csv": b"a,v\n1,1\n2,\n3,NA\n4,4\n",
    "spaces_plus.csv": b"v,w\n 42 ,+5\n",
    "overflow.csv": b"v\n99999999999999999999\n",
    "all_null.csv": b"a,v\n1,\n2,\\N\n",
    "strings_with_null.csv": b"s\nabc\nNA\n",
    "double_with_null.csv":b"a,x\n1,1.5\n2,\n"
}


for file in OUT_DIR.iterdir():
    if file.suffix == ".csv":
        file.unlink()
        print(f"deleting : {file.name}")

for name, data in FILES.items():
    (OUT_DIR / name).write_bytes(data)
    print(f"{name:26} {len(data):4} bytes")
