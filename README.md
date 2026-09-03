# reflect

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++26](https://img.shields.io/badge/C%2B%2B-26-blue.svg)]()
[![Header driven](https://img.shields.io/badge/header--driven-yes-brightgreen.svg)]()

## Usage

```cpp
#include "reflect/reflect.hpp"

struct [[=krrs::reflect_trait]] point
{
    int x;
    int y;
};

constexpr point p{3, 4};

std::cout << p;          // point{x: 3, y: 4}
std::format("{}", p);    // same, through std::formatter
```

C++26 makes reflection a built-in language feature. The `[[=krrs::reflect_trait]]` annotation taps
into it to give the type `operator<<` on `std::ostream` and `std::format` support — for free.

## What it is

A thin, header-driven wrapper over C++26 reflection. Opt a type in with a single annotation, and you
can print it, format it, and walk its members — without hand-writing any of that.

> [!NOTE]
> "Header-driven" applies to the core: `reflect`, `json`, and `argparse` are include-only. The
> `yaml` module is the one exception — it builds on `yaml-cpp`, which you link (e.g. via
> `FetchContent` or Conan).

---

## The core: `reflect/`

Tag a struct or enum with `[[=krrs::reflect_trait]]`. The type must be an **aggregate**.

```cpp
struct [[=krrs::reflect_trait]] circle
{
    std::string_view name;
    double radius;
};

enum class [[=krrs::reflect_trait]] color
{
    NONE = 0,
    red,
    green,
    blue,
};
```

**Print / format**

```cpp
constexpr circle c{.name = "unit", .radius = 2.5};
std::cout << c;                 // circle{name: unit, radius: 2.5}
std::format("{}", c);           // same
krrs::reflect::to_string(c);    // same, as std::string
```

**Enums** (need a `NONE` sentinel)

```cpp
static_assert(krrs::reflect::enum_to_string(color::green) == "green");
static_assert(krrs::reflect::string_to_enum<color>("blue") == color::blue);
```

Unmatched lookups fall back to `"UNKNOWN"` (by name) and `NONE` (by value).

**Iterate members** — `generate_nonstatic_member_metas<T>()` hands you the member list; drive it
with `template for` and read each member through the `obj.[:m:]` splice. It all stays `constexpr`:

```cpp
struct [[=krrs::reflect_trait]] vec3
{
    double x;
    double y;
    double z;
};

// generic over however many members the type declares
constexpr double dot(const vec3& a, const vec3& b)
{
    double sum = 0.0;
    static constexpr auto members = krrs::reflect::generate_nonstatic_member_metas<vec3>();
    template for (constexpr auto m : members)
    {
        sum += a.[:m:] * b.[:m:];
    }
    return sum;
}

static_assert(dot({1, 2, 3}, {4, 5, 6}) == 32.0);
```

By default the list **flattens base classes first**, then the derived members; pass
`generate_nonstatic_member_metas<T, false>()` for the current level only.

---

## YAML — `yaml/`

Read and write any reflected type as YAML. The type's name is the top-level key; its members are the
keys nested under it.

```cpp
#include "yaml/parser.hpp"

struct [[=krrs::reflect_trait]] server
{
    std::string host;
    int port;
    bool tls = false;   // has a default → optional on decode
};

const server s{.host = "0.0.0.0", .port = 8080};

const std::string text = krrs::yaml::serialize(s);
// server:
//   host: 0.0.0.0
//   port: 8080
//   tls: false

const server back = krrs::yaml::deserialize<server>(text);
```

Fields without a default are required. If any are missing from the YAML, `deserialize` throws and
names them all:

```cpp
const std::string incomplete = R"(
server:
  host: 0.0.0.0
)";

krrs::yaml::deserialize<server>(incomplete);   // port is required but absent
// throws: [yaml] missing the required keys: ["port"] for server
```

---

## JSON — `json/`

Serialise any reflected type to a JSON string. Nested reflected structs, containers, and
`std::optional` encode recursively — `nullopt` becomes `null`.

```cpp
#include "json/parser.hpp"

struct [[=krrs::reflect_trait]] account
{
    int id;
    std::string_view name;
    std::array<int, 2> roles;
    std::optional<int> tier;
};

constexpr account a{.id = 7, .name = "ada", .roles = {1, 2}, .tier = std::nullopt};

const std::string json = krrs::json::serialize(a);
// { "id": 7, "name": "ada", "roles": [1, 2], "tier": null }
```

`json` is serialise-only — there is no deserialisation back into a struct.

---

## CLI arguments — `argparse/`

Turn a reflected struct into a command-line parser — each field becomes a `--flag`. Like Python's
`argparse`, without the setup.

```cpp
#include "argparse/parser.hpp"

struct [[=krrs::reflect_trait]] options
{
    std::filesystem::path input;                  // required
    [[=krrs::shortform_trait]] int threads = 4;   // optional; -t via shortform
    std::vector<std::string> tags = {};           // optional, comma-separated
};

int main(int argc, const char* argv[])
{
    // ./app --input data.csv -t 8 --tags fast,verbose
    const auto args = krrs::argparse::parse_args<options>(argc, argv);
    if (!args)
    {
        std::cout << args.error() << '\n';   // --help / -h prints the generated usage here
        return 0;
    }

    const options& opts = *args;
    // opts.input, opts.threads, opts.tags ...
}
```

Fields without a default are required. If any are missing, `parse_args` throws
`std::invalid_argument` and names them all:

```cpp
const char* argv[] = {"app"};   // no --input
krrs::argparse::parse_args<options>(1, argv);
// throws: [argparse] missing required arguments: ["input"]
```

---

## License

MIT © KiryuRS
