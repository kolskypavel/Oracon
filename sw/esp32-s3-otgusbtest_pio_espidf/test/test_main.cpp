#include <unity.h>
#include <Arduino.h>
#include "protocol/protocol_parser.h"
#include <unity.h>
#include <Arduino.h>

void test_message_type_to_string(void) {
    ProtocolMessageType type = ProtocolMessageType::TYPE_ACK;
    std::string out = messageTypeToString(type);
    
    TEST_ASSERT_EQUAL_STRING(out.c_str(),"ACK");
}


void setup()
{
    delay(2000); // service delay
    UNITY_BEGIN();

    RUN_TEST(test_message_type_to_string);

    UNITY_END(); // stop unit testing
}

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}


void loop()
{
}