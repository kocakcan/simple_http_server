use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};

// ── Constants ────────────────────────────────────────────────────────────────
const PORT: u16 = 7878;
const BUF_SIZE: usize = 4096;
const MAX_HEADERS: usize = 32;
const MAX_HEADER_NAME: usize = 64;
const MAX_HEADER_VALUE: usize = 256;
const MAX_METHOD: usize = 8;
const MAX_PATH: usize = 256;
const MAX_VERSION: usize = 16;

// ── Data structures ──────────────────────────────────────────────────────────

// Rust strings are UTF-8 heap-allocated (String) or borrowed slices (&str).
// We mirror your fixed-size C arrays with owned Strings here for simplicity,
// but we could also use stack arrays like C — shown in comments below.
#[derive(Debug, Default)]
struct HttpHeader {
    name: String,  // C: char name[MAX_HEADER_NAME]
    value: String, // C: char value[MAX_HEADER_VALUE]
}

#[derive(Debug, Default)]
struct HttpRequest {
    method: String,  // C: char method[MAX_METHOD]
    path: String,    // C: char path[MAX_PATH]
    version: String, // C: char version[MAX_VERSION]
    headers: Vec<HttpHeader>, // C: struct http_header headers[MAX_HEADERS]
                     //    + int header_count  (Vec carries its own len)
}

// ── Request parser ───────────────────────────────────────────────────────────

// In C you returned -1 / 0.
// In Rust we return Result<T, E>: the compiler FORCES the caller to handle it.
fn parse_request(raw: &str) -> Result<HttpRequest, &'static str> {
    // No memset needed — Default::default() zero-initialises everything.
    let mut req = HttpRequest::default();

    // ── Request line ─────────────────────────────────────────────────────────
    // Find the first CRLF that terminates the request line.
    let line_end = raw.find("\r\n").ok_or("malformed: no CRLF")?;
    let request_line = &raw[..line_end];

    // Split "GET /path HTTP/1.1" on spaces — mirrors your strchr() dance.
    let mut parts = request_line.splitn(3, ' ');

    let method = parts.next().ok_or("malformed: missing method")?;
    let path = parts.next().ok_or("malformed: missing path")?;
    let version = parts.next().ok_or("malformed: missing version")?;

    // Enforce the same size limits you had in C.
    if method.len() >= MAX_METHOD {
        return Err("method too long");
    }
    if path.len() >= MAX_PATH {
        return Err("path too long");
    }
    if version.len() >= MAX_VERSION {
        return Err("version too long");
    }

    req.method = method.to_string();
    req.path = path.to_string();
    req.version = version.to_string();

    // ── Headers ───────────────────────────────────────────────────────────────
    // cursor advances through the raw bytes just like your `cursor` pointer.
    let mut cursor = &raw[line_end + 2..]; // skip past the first "\r\n"

    loop {
        let next_crlf = cursor.find("\r\n").ok_or("malformed: no CRLF in headers")?;

        if next_crlf == 0 {
            break;
        } // empty line → end of headers

        if req.headers.len() >= MAX_HEADERS {
            return Err("too many headers");
        }

        let header_line = &cursor[..next_crlf];

        // Find the colon separating name from value — mirrors your memchr().
        let colon = header_line
            .find(':')
            .ok_or("malformed: header has no colon")?;

        let name = &header_line[..colon];
        let value_raw = &header_line[colon + 1..];
        let value = value_raw.trim_start_matches(' '); // mirrors your while(*p==' ')

        if name.len() >= MAX_HEADER_NAME {
            return Err("header name too long");
        }
        if value.len() >= MAX_HEADER_VALUE {
            return Err("header value too long");
        }

        req.headers.push(HttpHeader {
            name: name.to_string(),
            value: value.to_string(),
        });

        cursor = &cursor[next_crlf + 2..]; // skip past "\r\n"
    }

    Ok(req) // ← explicit success value; no magic 0 / -1
}

// ── Connection handler ───────────────────────────────────────────────────────

// Extracted into its own function to keep main() clean.
// Takes ownership of the TcpStream; it is automatically closed (Drop) when
// this function returns — no explicit close(client_fd) needed.
fn handle_client(mut stream: TcpStream) {
    let peer = stream
        .peer_addr()
        .map(|a| a.to_string())
        .unwrap_or_else(|_| "unknown".into());

    println!("Got a connection! (peer={})", peer);

    // ── Read ──────────────────────────────────────────────────────────────────
    let mut buf = [0u8; BUF_SIZE];
    let n = match stream.read(&mut buf) {
        Ok(n) => n,
        Err(e) => {
            eprintln!("read error: {}", e);
            return;
        }
    };

    // Convert raw bytes → &str (UTF-8).
    // from_utf8() is the safe alternative to just casting in C.
    let raw = match std::str::from_utf8(&buf[..n]) {
        Ok(s) => s,
        Err(_) => {
            eprintln!("request is not valid UTF-8");
            let _ = stream.write_all(
                b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
            );
            return;
        }
    };

    // ── Parse ─────────────────────────────────────────────────────────────────
    let req = match parse_request(raw) {
        Ok(r) => r,
        Err(e) => {
            eprintln!("Malformed request ({}), sending 400", e);
            let _ = stream.write_all(
                b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n",
            );
            return;
        }
    };

    // ── Log ───────────────────────────────────────────────────────────────────
    println!(
        "Method: {} | Path: {} | Version: {} | Headers: {}",
        req.method,
        req.path,
        req.version,
        req.headers.len()
    );
    for h in &req.headers {
        println!("  [{}] = [{}]", h.name, h.value);
    }

    // ── Respond ───────────────────────────────────────────────────────────────
    let response = "HTTP/1.1 200 OK\r\n\
         Content-Type: text/plain\r\n\
         Content-Length: 13\r\n\
         Connection: close\r\n\
         \r\n\
         Hello, World!\n";

    if let Err(e) = stream.write_all(response.as_bytes()) {
        eprintln!("write error: {}", e);
    }
    // stream goes out of scope here → Drop closes the socket automatically.
}

// ── main ─────────────────────────────────────────────────────────────────────
fn main() {
    // TcpListener wraps socket() + setsockopt(SO_REUSEADDR) + bind() + listen()
    // in one safe call.  No manual fd management.
    let listener = TcpListener::bind(("127.0.0.1", PORT)).unwrap_or_else(|e| {
        eprintln!("bind error: {}", e);
        std::process::exit(1);
    });

    println!("Server listening on http://127.0.0.1:{}", PORT);

    // accept() loop — mirrors your while(1) { accept(...) }
    for stream in listener.incoming() {
        match stream {
            Ok(s) => handle_client(s),
            Err(e) => eprintln!("accept error: {}", e),
        }
    }
    // listener goes out of scope → socket closed automatically (unreachable,
    // but handled for free — just like your close(sockfd) comment).
}
