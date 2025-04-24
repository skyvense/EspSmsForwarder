#include <stdio.h>
#include "../src/pdu.h"

void setUp(void) {
    // setup code
}

void tearDown(void) {
    // teardown code
}

void test_decodeHexContent_chinese() {
    PDUHelper helper;
    // 6D4B = 测, 8BD5 = 试, 7ED3 = 结, 679C = 果, 663E = 显, 793A = 示
    String hex = "6D4B8BD57ED3679C663E793A";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "测试结果显示") {
        printf("Test failed: Chinese decode\n");
        printf("Expected: 测试结果显示\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Chinese decode\n");
    }
}

void test_decodeHexContent_ascii() {
    PDUHelper helper;
    // 0048 = H, 0065 = e, 006C = l, 006C = l, 006F = o, 0020 = space, 0057 = W, 006F = o, 0072 = r, 006C = l, 0064 = d
    String hex = "00480065006C006C006F00200057006F0072006C0064";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "Hello World") {
        printf("Test failed: ASCII decode\n");
        printf("Expected: Hello World\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII decode\n");
    }
}

void test_decodeHexContent_ascii_direct() {
    PDUHelper helper;
    // 直接使用ASCII字符串的十六进制表示
    String hex = "68656C6C6F2C20776F726C642C20616263";  // "hello, world, abc"
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "hello, world, abc") {
        printf("Test failed: ASCII direct decode\n");
        printf("Expected: hello, world, abc\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII direct decode\n");
    }
}

void test_decodeHexContent_ascii_mixed() {
    PDUHelper helper;
    // 混合ASCII和特殊字符
    String hex = "48656C6C6F2021576F726C6421204040232425";  // "Hello !World!!@#$%"
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "Hello !World!!@#$%") {
        printf("Test failed: ASCII mixed decode\n");
        printf("Expected: Hello !World!!@#$%\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII mixed decode\n");
    }
}

void test_decodeHexContent_ascii_raw() {
    PDUHelper helper;
    // 纯ASCII字符串，不进行任何编码转换
    String hex = "Hello World";  // 直接使用ASCII字符串
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "Hello World") {
        printf("Test failed: ASCII raw decode\n");
        printf("Expected: Hello World\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII raw decode\n");
    }
}

void test_decodeHexContent_empty() {
    PDUHelper helper;
    String hex = "";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "") {
        printf("Test failed: Empty string decode\n");
        printf("Expected: (empty)\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Empty string decode\n");
    }
}

void test_decodeHexContent_invalid() {
    PDUHelper helper;
    // 6D4B = 测, 8BD5 = 试, 7ED3 = 结, 679C = 果, 663E = 显 (不完整)
    String hex = "6D4B8BD57ED3679C663E";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "测试结果显") {
        printf("Test failed: Invalid hex decode\n");
        printf("Expected: 测试结果显\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Invalid hex decode\n");
    }
}

void test_decodeHexContent_mixed() {
    PDUHelper helper;
    // 6D4B = 测, 8BD5 = 试, 0048 = H, 0065 = e, 006C = l, 006C = l, 006F = o
    String hex = "6D4B8BD500480065006C006C006F";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "测试Hello") {
        printf("Test failed: Mixed content decode\n");
        printf("Expected: 测试Hello\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Mixed content decode\n");
    }
}

void test_decodeHexContent_special() {
    PDUHelper helper;
    // 0021 = !, 0023 = #, 0025 = %, 0026 = &, 002F = /, 003F = ?, 0040 = @
    String hex = "0021002300250026002F003F0040";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "!#%&/?@") {
        printf("Test failed: Special characters decode\n");
        printf("Expected: !#%&/?@\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Special characters decode\n");
    }
}

void test_decodeHexContent_long() {
    PDUHelper helper;
    // 4F60 = 你, 597D = 好 (重复8次)
    String hex = "4F60597D4F60597D4F60597D4F60597D4F60597D4F60597D4F60597D4F60597D";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "你好你好你好你好你好你好你好你好") {
        printf("Test failed: Long string decode\n");
        printf("Expected: 你好你好你好你好你好你好你好你好\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Long string decode\n");
    }
}

void test_decodeHexContent_odd_length() {
    PDUHelper helper;
    // 0048 = H, 0065 = e, 006C = l, 006C = l, 006F = o, 0020 = space, 0057 = W, 006F = o, 0072 = r, 006C = l (不完整)
    String hex = "00480065006C006C006F00200057006F0072006C";
    String result = PDUHelper::decodeHexToString(hex);
    if (result != "Hello Worl") {
        printf("Test failed: Odd length decode\n");
        printf("Expected: Hello Worl\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Odd length decode\n");
    }
}

void test_decodeContent_chinese() {
    PDUHelper helper;
    // 6D4B = 测, 8BD5 = 试, 7ED3 = 结, 679C = 果, 663E = 显, 793A = 示
    String hex = "6D4B8BD57ED3679C663E793A";
    String result = PDUHelper::decodeContent(hex);
    if (result != "测试结果显示") {
        printf("Test failed: Chinese decode\n");
        printf("Expected: 测试结果显示\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Chinese decode\n");
    }
}

void test_decodeContent_ascii() {
    PDUHelper helper;
    // 直接使用ASCII字符串
    String content = "Hello World";
    String result = PDUHelper::decodeContent(content);
    if (result != "Hello World") {
        printf("Test failed: ASCII content\n");
        printf("Expected: Hello World\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII content\n");
    }
}

void test_decodeContent_ascii_hex() {
    PDUHelper helper;
    // ASCII字符串的十六进制表示
    String hex = "48656C6C6F20576F726C64";  // "Hello World"
    String result = PDUHelper::decodeContent(hex);
    if (result != "Hello World") {
        printf("Test failed: ASCII hex decode\n");
        printf("Expected: Hello World\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: ASCII hex decode\n");
    }
}

void test_decodeContent_mixed() {
    PDUHelper helper;
    // 混合内容：ASCII字符串
    String content = "Hello 测试 World";
    String result = PDUHelper::decodeContent(content);
    if (result != "Hello 测试 World") {
        printf("Test failed: Mixed content\n");
        printf("Expected: Hello 测试 World\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Mixed content\n");
    }
}

void test_decodeContent_empty() {
    PDUHelper helper;
    String content = "";
    String result = PDUHelper::decodeContent(content);
    if (result != "") {
        printf("Test failed: Empty content\n");
        printf("Expected: (empty)\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Empty content\n");
    }
}

void test_decodeContent_special() {
    PDUHelper helper;
    // 特殊字符
    String content = "Hello! @#$%^&*()_+";
    String result = PDUHelper::decodeContent(content);
    if (result != "Hello! @#$%^&*()_+") {
        printf("Test failed: Special characters\n");
        printf("Expected: Hello! @#$%^&*()_+\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Special characters\n");
    }
}

void test_decodeContent_mixed_encoding() {
    PDUHelper helper;
    // 混合编码：ASCII + 中文
    // 0061 = a, 0062 = b, 0063 = c, 0020 = 空格, 6D4B = 测, 8BD5 = 试, 7ED3 = 结, 679C = 果, 663E = 显, 793A = 示
    String hex = "00610062006300206D4B8BD57ED3679C663E793A";
    String result = PDUHelper::decodeContent(hex);
    if (result != "abc 测试结果显示") {
        printf("Test failed: Mixed encoding decode\n");
        printf("Expected: abc 测试结果显示\n");
        printf("Got: %s\n", result.c_str());
    } else {
        printf("Test passed: Mixed encoding decode\n");
    }
}

int main() {
    printf("Running PDU decode tests...\n\n");
    
    test_decodeContent_chinese();
    test_decodeContent_ascii();
    test_decodeContent_ascii_hex();
    test_decodeContent_mixed();
    test_decodeContent_empty();
    test_decodeContent_special();
    test_decodeContent_mixed_encoding();
    
    printf("\nAll tests completed.\n");
    return 0;
} 