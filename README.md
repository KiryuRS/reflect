# KRRS Reflect

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++26](https://img.shields.io/badge/C%2B%2B-26-blue.svg)]()

## Motivation

> [!NOTE]
> By no means this is meant to be a replacement of C++26's reflection feature.  
> This is really more of an extension + wrapper for ease without having to write the same boiler code

I use reflection based system in both personal and professional codebases on a daily basis, its becoming like a bread-and-butter.  
However, these reflections are built upon macros _(with `BOOST_PP` extensions)_ and it comes with downsides. For starters:

* Macros - urgh... I am very bad at using macros
* While using `BOOST_DESCRIBE` works as a replacement, there are already 2 versions of it, and we are constrained to its API. Errors that occur within the lambda during the `for_each` function wrapper gives you compiler errors nightmare
* For any iteration required, there's really no way but to use a "for-each with lambda" approach

### Pre CPP26

There's a custom reflection library that I've made in `precpp26` branch which utilizes on C++23's technique while trying to mimic C++26's reflection as much as possible _(e.g. `std::meta::info`)_.  

But in comparison, the C++26's version _(in this branch)_ felt more of a steriod version and improvements that otherwise couldn't be made possible. See next section for illustration

## Extensions and Features

The following extensions were done with C++26's reflections:

* `enum` <-> string conversions
* Generating non-static member "descriptors"
* Generating function "descriptors"
* Stringize version of a reflected class
* Validating if 2 template types are the same, without explicitly comparing its arguments

And these features were improved / added upon:

* `yaml` <-> cpp conversion with a single function call
* `argparse` with just C++ struct as the setup, with validators
* cpp -> `json` string encoding _(by no means im comparing it with Glaze library)_
* `inteoc` - a feature which could be used to create type-erased types based on interfaces. basically compile-time polymorphism

> [!WARNING]
> With the exception of enums, all features that will be explained below require some annotations to the user defined type itself.  
> This is intended to prevent accidental reflection code compiled for STL or third-party class types

## Core Reflection

The idea in this library is to provide the simplest user experience for enabling reflection codebase. First, to show an example of how it looks like:

```c++
#include "reflect/reflect.hpp"

// user is only required to add this annotation.
// possible to macro it as:
// #define REFLECT_TRAIT [[=krrs::reflect_trait]]
struct [[=krrs::reflect_trait]] Point2D
{
    int x;
    int y;
}

constexpr Point2D coordinate{100, 200};

// print - either one of the following
std::cout << std::format("Coordinate is at: {}", p);
std::cout << "Coordinate is at: " << p;
std::cout << "Coordinate is at: " << krrs::reflect::to_string(p);

// or iterate the members
template for (constexpr auto member : krrs::reflect::generate_nonstatic_member_metas<Point2D>())
{
    using type = [:std::meta::type_of(member):];
    if constexpr (std::meta::identifier_of(member) == "x")
    {
        static_assert(std::same_as<type, int>);
    }
    else if constexpr (std::meta::identifier_of(member) == "y")
    {
        static_assert(std::same_as<type, int>);
    }
}
```

For enums, it looks even more "magic", but the **caveat** is that it needs a `NONE` field:

```c++
enum class Generation
{
    NONE,
    GEN_X,
    GEN_Y,
    GEN_Z,
    OTHERS,
};

constexpr Generation g = Generation::GEN_X;

// enum <-> string
static_assert(enum_to_string(g) == "GEN_X");
static_assert(string_to_enum<Generation>("GEN_X") == Generation::GEN_X);s

// print - either one of the following
std::cout << enum_to_string(g);
std::cout << std::format("{}", g);
```

### Extras

#### `instance_of`

The problem with template types are that its difficult to "substitute" template arguments from one to another to verify if e.g. X == Y == `std::array`, because if one of them is a `std::vector`, the substitution just couldn't happen (e.g. second argument of `std::array` is NTTP).  

`instance_of` solves this problem and the following would compile and work as intended:

```c++
std::vector<std::string> v;
std::array<int, 10> a;

static_assert(instance_of<std::vector<std::string>, ^^v>);
static_assert(!instance_of<std::vector<int>, ^^a>);
static_assert(instance_of<std::array<int, 10>, ^^a>);
```

For more details, refer to `tests/test_reflections.cpp`

## Features

These are additional features that you get for free when including this library

### YAML <-> CPP

> [!WARNING]
> This requires external library [yaml-cpp](https://github.com/jbeder/yaml-cpp)  
> At time of writing - the version used is 0.8.0

Example usage:

```c++
#include "yaml/parser.hpp"

struct [[=krrs::reflect_trait]] Server
{
    std::string host;
    int port;
    bool tls = false; // notice the default field here
};

constexpr Server server{.host = "0.0.0.0", .port = 8080};

const std::string yaml_str = krrs::yaml::serialize(server);
// server:
//   host: 0.0.0.0
//   port: 8080
//   tls: false

const std::string config_str = R"(
server:
    host: 0.0.0.0
    port: 8080
)";

const Server server_conf = krrs::yaml::deserialize<Server>(config_str);
assert(server_conf == server);


// variables without default fields will invoke a runtime error if it cannot deduce the deserialization logic

const std::string config_str_2 = R"(
server:

)";

krrs::yaml::deserialize<Server>(config_str_2);
// std::runtime_error - [yaml] missing the required keys: ["host", "port"] for Server
```

For more details, refer to `tests/test_yaml_parser.cpp`

### `argparse`

Inspired from Python's `argparse`, except the setup here is much more straightforward. Validators are also supported through annotations, alongside with default "arguments" and short form flags.  

An example below of how it looks like:

```c++
#include "argparse/parser.hpp"

struct [[=krrs::reflect_trait]] Options
{
    std::filesystem::path input;
    [[=krrs::shortform_trait]] int threads = 4;
    std::vector<std::string> tags = {};
};

// In python, imagine it with @click.option:
// @click.option("--input", "input", type=click.Path, required=True)
// @click.option("--threads", "-t", "threads", default=4, type=click.Int, required=False)
// @click.option("--tags", default=[], type=<vector of string>, required=False)

int main(int argc, const char* argv[])
{
    // example of CLI:
    // ./app --input data.csv -t 8 --tags fast,verbose
    const auto args = krrs::argparse::parse_args<Options>(argc, argv);
    if (!args)
    {
        // --help or -h would also invoke this section
        std::cout << args.error() << '\n';
        return 0;
    }

    const Options opts = *args;
    // opts.input = data.csv
    // opts.threads = 8
    // opts.tags = ["fast", "verbose"]


    // similar to yaml parser, any missing arguments will invoke a runtime error
    const auto missing_argv[] = {"app"};
    krrs::argparse::parse_args<Options>(1, argv);
    // std::runtime_error - [argparse] missing required arguments: ["input"]
}
```

For more details, refer to `tests/test_argparse.cpp`

### CPP -> JSON

Only performs serialization from any reflected type to a JSON string.  
It supports for nested reflected types, containers.  
For `std::optional`, if there's no value supplied, represents it as `null`.

```c++
#include "json/parser.hpp"

struct [[=krrs::reflect_trait]] Account
{
    int id;
    std::string_view name;
    std::array<int, 2> roles;
    std::optional<int> tier;
};

constexpr Account a1{.id = 7, .name = "ada", .roles = {1, 2}, .tier = std::nullopt};

const std::string json = krrs::json::serialize(a1);
// { "id": 7, "name": "ada", "roles": [1, 2], "tier": null }
```

For more details, refer to `tests/test_json_parser.cpp`

# License

MIT © KiryuRS
