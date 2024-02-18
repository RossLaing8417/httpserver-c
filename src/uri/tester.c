#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "uri.h"

#define TEST(NAME) static void NAME(void **state)

Uri tryParseUri(char * source) {
    UriOrErr uri_err = parseUri(source, strlen(source));
    assert_int_equal(OPTION_SOME, uri_err.option);
    return uri_err.value;
}

CharSlice unwrapStrOpt(StrOpt opt) {
    assert_int_equal(OPTION_SOME, opt.option);
    return opt.some;
}

uint16_t unwrapu16Opt(u16Opt opt) {
    assert_int_equal(OPTION_SOME, opt.option);
    return opt.some;
}

void expectEqualString2CharSlice(char * expected, CharSlice actual) {
    char buffer[actual.len + 1];
    strncpy(buffer, actual.ptr, actual.len);
    buffer[actual.len] = '\0';
    assert_string_equal(expected, buffer);
}

void expectShallowEqualCharSlice(CharSlice expected, CharSlice actual) {
    assert_ptr_equal(expected.ptr, actual.ptr);
    assert_ptr_equal(expected.len, actual.len);
}

void expectShallowEqualStrOpt(StrOpt expected, StrOpt actual) {
    assert_int_equal(expected.option, actual.option);
    if (expected.option == OPTION_SOME) {
        expectShallowEqualCharSlice(expected.some, actual.some);
    }
}

void expectShallowEqualu16Opt(u16Opt expected, u16Opt actual) {
    assert_int_equal(expected.option, actual.option);
    if (expected.option == OPTION_SOME) {
        assert_int_equal(expected.some, actual.some);
    }
}

void expectShallowEqualUri(Uri expected, Uri actual) {
    expectShallowEqualCharSlice(expected.scheme, actual.scheme);
    expectShallowEqualStrOpt(expected.user, actual.user);
    expectShallowEqualStrOpt(expected.password, actual.password);
    expectShallowEqualStrOpt(expected.host, actual.host);
    expectShallowEqualu16Opt(expected.port, actual.port);
    expectShallowEqualStrOpt(expected.path, actual.path);
    expectShallowEqualStrOpt(expected.query, actual.query);
    expectShallowEqualStrOpt(expected.fragment, actual.fragment);
}

TEST(basic) {
    (void) state;

    Uri uri = tryParseUri("https://ziglang.org/download");

    expectEqualString2CharSlice("https", uri.scheme);
    expectEqualString2CharSlice("ziglang.org", unwrapStrOpt(uri.host));
    expectEqualString2CharSlice("/download", unwrapStrOpt(uri.path));
    assert_int_equal(OPTION_NONE, uri.port.option);
}

TEST(withPort) {
    (void) state;

    Uri uri = tryParseUri("http://example:1337/");

    expectEqualString2CharSlice("http", uri.scheme);
    expectEqualString2CharSlice("example", uri.host.some);
    expectEqualString2CharSlice("/", uri.path.some);
    assert_int_equal(1337, uri.port.some);
}

TEST(parseFail) {
    (void) state;

    char * source = "foobar://";
    UriOrErr err = parseUri(source, strlen(source));
    assert_int_equal(OPTION_ERROR, err.option);
    assert_int_equal(URI_ERROR_BAD_FORMAT, err.error);
}

TEST(scheme) {
    (void) state;

    expectEqualString2CharSlice("http", tryParseUri("http:_").scheme);
    expectEqualString2CharSlice("scheme-mee", tryParseUri("scheme-mee:_").scheme);
    expectEqualString2CharSlice("http", tryParseUri("http:_").scheme);
    expectEqualString2CharSlice("scheme-mee", tryParseUri("scheme-mee:_").scheme);
    expectEqualString2CharSlice("a.b.c", tryParseUri("a.b.c:_").scheme);
    expectEqualString2CharSlice("ab+", tryParseUri("ab+:_").scheme);
    expectEqualString2CharSlice("X+++", tryParseUri("X+++:_").scheme);
    expectEqualString2CharSlice("Y+-.", tryParseUri("Y+-.:_").scheme);
}

TEST(authority) {
    (void) state;

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://hostname").host));

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://userinfo@hostname").host));
    expectEqualString2CharSlice("userinfo", unwrapStrOpt(tryParseUri("scheme://userinfo@hostname").user));
    assert_int_equal(OPTION_NONE, tryParseUri("scheme://userinfo@hostname").password.option);

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://user:password@hostname").host));
    expectEqualString2CharSlice("user", unwrapStrOpt(tryParseUri("scheme://user:password@hostname").user));
    expectEqualString2CharSlice("password", unwrapStrOpt(tryParseUri("scheme://user:password@hostname").password));

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://hostname:0").host));
    assert_int_equal(1234, unwrapu16Opt(tryParseUri("scheme://hostname:1234").port));

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://userinfo@hostname:1234").host));
    assert_int_equal(1234, unwrapu16Opt(tryParseUri("scheme://userinfo@hostname:1234").port));
    expectEqualString2CharSlice("userinfo", unwrapStrOpt(tryParseUri("scheme://userinfo@hostname:1234").user));
    assert_int_equal(OPTION_NONE, tryParseUri("scheme://userinfo@hostname:1234").password.option);

    expectEqualString2CharSlice("hostname", unwrapStrOpt(tryParseUri("scheme://user:password@hostname:1234").host));
    assert_int_equal(1234, unwrapu16Opt(tryParseUri("scheme://user:password@hostname:1234").port));
    expectEqualString2CharSlice("user", unwrapStrOpt(tryParseUri("scheme://user:password@hostname:1234").user));
    expectEqualString2CharSlice("password", unwrapStrOpt(tryParseUri("scheme://user:password@hostname:1234").password));
}

TEST(authorityPassword) {
    (void) state;
    expectEqualString2CharSlice("username", unwrapStrOpt(tryParseUri("scheme://username@a").user));
    assert_int_equal(OPTION_NONE, tryParseUri("scheme://username@a").password.option);

    expectEqualString2CharSlice("username", unwrapStrOpt(tryParseUri("scheme://username:@a").user));
    assert_int_equal(OPTION_NONE, tryParseUri("scheme://username:@a").password.option);

    expectEqualString2CharSlice("username", unwrapStrOpt(tryParseUri("scheme://username:password@a").user));
    expectEqualString2CharSlice("password", unwrapStrOpt(tryParseUri("scheme://username:password@a").password));

    expectEqualString2CharSlice("username", unwrapStrOpt(tryParseUri("scheme://username::@a").user));
    expectEqualString2CharSlice(":", unwrapStrOpt(tryParseUri("scheme://username::@a").password));
}

#define TEST_AUTHORITY_HOST(HOST) expectEqualString2CharSlice(HOST, unwrapStrOpt(tryParseUri("scheme://" HOST).host));

TEST(authorityDnsNames) {
    (void) state;
    TEST_AUTHORITY_HOST("a");
    TEST_AUTHORITY_HOST("a.b");
    TEST_AUTHORITY_HOST("example.com");
    TEST_AUTHORITY_HOST("www.example.com");
    TEST_AUTHORITY_HOST("example.org.");
    TEST_AUTHORITY_HOST("www.example.org.");
    TEST_AUTHORITY_HOST("xn--nw2a.xn--j6w193g");
    TEST_AUTHORITY_HOST("fe80--1ff-fe23-4567-890as3.ipv6-literal.net");
}

TEST(authorityIPv4) {
    (void) state;
    TEST_AUTHORITY_HOST("127.0.0.1");
    TEST_AUTHORITY_HOST("255.255.255.255");
    TEST_AUTHORITY_HOST("0.0.0.0");
    TEST_AUTHORITY_HOST("8.8.8.8");
    TEST_AUTHORITY_HOST("1.2.3.4");
    TEST_AUTHORITY_HOST("192.168.0.1");
    TEST_AUTHORITY_HOST("10.42.0.0");
}

TEST(authorityIPv6) {
    (void) state;
    TEST_AUTHORITY_HOST("[2001:db8:0:0:0:0:2:1]");
    TEST_AUTHORITY_HOST("[2001:db8::2:1]");
    TEST_AUTHORITY_HOST("[2001:db8:0000:1:1:1:1:1]");
    TEST_AUTHORITY_HOST("[2001:db8:0:1:1:1:1:1]");
    TEST_AUTHORITY_HOST("[0:0:0:0:0:0:0:0]");
    TEST_AUTHORITY_HOST("[0:0:0:0:0:0:0:1]");
    TEST_AUTHORITY_HOST("[::1]");
    TEST_AUTHORITY_HOST("[::]");
    TEST_AUTHORITY_HOST("[2001:db8:85a3:8d3:1319:8a2e:370:7348]");
    TEST_AUTHORITY_HOST("[fe80::1ff:fe23:4567:890a%25eth2]");
    TEST_AUTHORITY_HOST("[fe80::1ff:fe23:4567:890a]");
    TEST_AUTHORITY_HOST("[fe80::1ff:fe23:4567:890a%253]");
    TEST_AUTHORITY_HOST("[fe80:3::1ff:fe23:4567:890a]");
}

TEST(rfcExample1) {
    (void) state;

    char * source = "foo://example.com:8042/over/there?name=ferret#nose";
    Uri uri = {
        .scheme = { .ptr = source, .len = 3 },
        .user = AS_NONE(),
        .password = AS_NONE(),
        .host = AS_SOME({ .ptr = source + 6, .len = 17 - 6 }),
        .port = AS_SOME(8042),
        .path = AS_SOME({ .ptr = source + 22, .len = 33 - 22 }),
        .query = AS_SOME({ .ptr = source + 34, .len = 45 - 34 }),
        .fragment = AS_SOME({ .ptr = source + 46, .len = 50 - 46 }),
    };

    expectShallowEqualUri(uri, tryParseUri(source));
}

TEST(rfcExample2) {
    (void) state;

    char * source = "urn:example:animal:ferret:nose";
    Uri uri = {
        .scheme = { .ptr = source, .len = 3 },
        .user = AS_NONE(),
        .password = AS_NONE(),
        .host = AS_NONE(),
        .port = AS_NONE(),
        .path = AS_SOME({ .ptr = source + 4, .len = strlen(source) - 4 }),
        .query = AS_NONE(),
        .fragment = AS_NONE(),
    };

    expectShallowEqualUri(uri, tryParseUri(source));
}

TEST(wikipediaExamples) {
    (void) state;
    tryParseUri("https://john.doe@www.example.com:123/forum/questions/?tag=networking&order=newest#top");
    tryParseUri("ldap://[2001:db8::7]/c=GB?objectClass?one");
    tryParseUri("mailto:John.Doe@example.com");
    tryParseUri("news:comp.infosystems.www.servers.unix");
    tryParseUri("tel:+1-816-555-1212");
    tryParseUri("telnet://192.0.2.16:80/");
    tryParseUri("urn:oasis:names:specification:docbook:dtd:xml:4.1.2");
    tryParseUri("http://a/b/c/d;p?q");
}

TEST(rfcExamples) {
    (void) state;
    tryParseUri("http://a/b/c/g");
    tryParseUri("http://a/b/c/g");
    tryParseUri("http://a/b/c/g/");
    tryParseUri("http://a/g");
    tryParseUri("http://g");
    tryParseUri("http://a/b/c/d;p?y");
    tryParseUri("http://a/b/c/g?y");
    tryParseUri("http://a/b/c/d;p?q#s");
    tryParseUri("http://a/b/c/g#s");
    tryParseUri("http://a/b/c/g?y#s");
    tryParseUri("http://a/b/c/;x");
    tryParseUri("http://a/b/c/g;x");
    tryParseUri("http://a/b/c/g;x?y#s");
    tryParseUri("http://a/b/c/d;p?q");
    tryParseUri("http://a/b/c/");
    tryParseUri("http://a/b/c/");
    tryParseUri("http://a/b/");
    tryParseUri("http://a/b/");
    tryParseUri("http://a/b/g");
    tryParseUri("http://a/");
    tryParseUri("http://a/");
    tryParseUri("http://a/g");
}

TEST(specialTest) {
    (void) state;

    tryParseUri("https://www.youtube.com/watch?v=dQw4w9WgXcQ&feature=youtu.be&t=0");
}

// test "URI escaping" {
//     const input = "\\ö/ äöß ~~.adas-https://canvas:123/#ads&&sad";
//     const expected = "%5C%C3%B6%2F%20%C3%A4%C3%B6%C3%9F%20~~.adas-https%3A%2F%2Fcanvas%3A123%2F%23ads%26%26sad";
//
//     const actual = try escapeString(std.testing.allocator, input);
//     defer std.testing.allocator.free(actual);
//
//     try std.testing.expectEqualSlices(u8, expected, actual);
// }

// test "URI unescaping" {
//     const input = "%5C%C3%B6%2F%20%C3%A4%C3%B6%C3%9F%20~~.adas-https%3A%2F%2Fcanvas%3A123%2F%23ads%26%26sad";
//     const expected = "\\ö/ äöß ~~.adas-https://canvas:123/#ads&&sad";
//
//     const actual = try unescapeString(std.testing.allocator, input);
//     defer std.testing.allocator.free(actual);
//
//     try std.testing.expectEqualSlices(u8, expected, actual);
// }

// test "URI query escaping" {
//     const address = "https://objects.githubusercontent.com/?response-content-type=application%2Foctet-stream";
//     const parsed = try Uri.parse(address);
//
//     // format the URI to escape it
//     const formatted_uri = try std.fmt.allocPrint(std.testing.allocator, "{}", .{parsed});
//     defer std.testing.allocator.free(formatted_uri);
//     try std.testing.expectEqualStrings("/?response-content-type=application%2Foctet-stream", formatted_uri);
// }

int main(int argc, char *argv[]) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(basic),
        cmocka_unit_test(withPort),
        cmocka_unit_test(parseFail),
        cmocka_unit_test(scheme),
        cmocka_unit_test(authority),
        cmocka_unit_test(authorityPassword),
        cmocka_unit_test(authorityDnsNames),
        cmocka_unit_test(authorityIPv4),
        cmocka_unit_test(authorityIPv6),
        cmocka_unit_test(rfcExample1),
        cmocka_unit_test(rfcExample2),
        cmocka_unit_test(wikipediaExamples),
        cmocka_unit_test(rfcExamples),
        cmocka_unit_test(specialTest),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
