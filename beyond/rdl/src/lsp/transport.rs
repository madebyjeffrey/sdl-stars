use std::io::{self, BufRead, Read, Write};

use serde_json::Value;

const MAX_BODY: usize = 16 * 1024 * 1024;
const MAX_HEADER: usize = 8192;

pub(super) fn read(reader: &mut impl BufRead) -> io::Result<Option<Vec<u8>>> {
    let mut length = None;
    let mut header_size = 0;
    loop {
        let mut line = Vec::new();
        let count = reader
            .take((MAX_HEADER + 1) as u64)
            .read_until(b'\n', &mut line)?;
        if count == 0 && header_size == 0 {
            return Ok(None);
        }
        header_size += count;
        if header_size > MAX_HEADER || !line.ends_with(b"\r\n") {
            return Err(invalid("invalid or oversized LSP header"));
        }
        if line == b"\r\n" {
            break;
        }
        let line = std::str::from_utf8(&line).map_err(|_| invalid("non-ASCII LSP header"))?;
        if !line.is_ascii() {
            return Err(invalid("non-ASCII LSP header"));
        }
        let (name, value) = line
            .trim_end()
            .split_once(':')
            .ok_or_else(|| invalid("invalid LSP header"))?;
        if name.eq_ignore_ascii_case("Content-Length") {
            if length.is_some() {
                return Err(invalid("duplicate Content-Length"));
            }
            let count: usize = value
                .trim()
                .parse()
                .map_err(|_| invalid("invalid Content-Length"))?;
            if count > MAX_BODY {
                return Err(invalid("LSP message exceeds 16 MiB"));
            }
            length = Some(count);
        }
        if name.eq_ignore_ascii_case("Content-Type")
            && let Some((_, charset)) = value.to_ascii_lowercase().split_once("charset=")
            && !matches!(charset.trim(), "utf-8" | "utf8")
        {
            return Err(invalid("LSP messages must use UTF-8"));
        }
    }
    let mut body = vec![0; length.ok_or_else(|| invalid("missing Content-Length"))?];
    reader.read_exact(&mut body)?;
    Ok(Some(body))
}

pub(super) fn write(writer: &mut impl Write, value: &Value) -> io::Result<()> {
    let body = serde_json::to_vec(value)?;
    write!(writer, "Content-Length: {}\r\n\r\n", body.len())?;
    writer.write_all(&body)?;
    writer.flush()
}

fn invalid(message: &str) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message)
}
