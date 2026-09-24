"""Generate the Themis CSV torture corpus.

Run from the project root:  python3 data/corpus/make_corpus.py
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
    "quote_mid_field.csv": b'ab\"cd,e'
}

#TODO: Add a funtion that clean evertyhing before creating the new files

for name, data in FILES.items():
    (HERE / name).write_bytes(data)
    print(f"{name:24} {len(data):4} bytes")
