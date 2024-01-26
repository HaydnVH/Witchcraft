#include "base64.h"
#include "dbg/debug.h"
#include "dbg/unittest.h"

#include <format>

// Performs a test to convert a string into an expected base64 output and then
// back again.
auto singleTest(std::string_view input, std::string_view output) {
  int  FAIL_COUNTER {0};
  auto converted1 = wc::base64::encode(input.data(), input.size());
  EXPECT_EQUAL(converted1, output);
  auto converted2  = wc::base64::decode(converted1);
  auto converted2s = std::string(converted2.begin(), converted2.end());
  EXPECT_EQUAL(converted2s, input);
  return FAIL_COUNTER;
}

int wc::base64::test::runBase64UnitTests() {
  int FAIL_COUNTER {0};

  FAIL_COUNTER +=
      singleTest("This is a test string.", "VGhpcyBpcyBhIHRlc3Qgc3RyaW5nLg");
  FAIL_COUNTER +=
      singleTest("This is a test string!", "VGhpcyBpcyBhIHRlc3Qgc3RyaW5nIQ");
  FAIL_COUNTER += singleTest("Another Test String", "QW5vdGhlciBUZXN0IFN0cmluZw");
  FAIL_COUNTER += singleTest(
      "Lorem Ipsum is simply dummy text of the printing and typesetting "
      "industry. Lorem Ipsum has been the industry's standard dummy text "
      "ever since the 1500s, when an unknown printer took a galley of type "
      "and scrambled it to make a type specimen book. It has survived not "
      "only five centuries, but also the leap into electronic typesetting, "
      "remaining essentially unchanged. It was popularised in the 1960s with "
      "the release of Letraset sheets containing Lorem Ipsum passages, and "
      "more recently with desktop publishing software like Aldus PageMaker "
      "including versions of Lorem Ipsum.",
      "TG9yZW0gSXBzdW0gaXMgc2ltcGx5IGR1bW15IHRleHQgb2YgdGhlIHByaW50aW5nIGFuZCB"
      "0eXBlc2V0dGluZyBpbmR1c3RyeS4gTG9yZW0gSXBzdW0gaGFzIGJlZW4gdGhlIGluZHVzdH"
      "J5J3Mgc3RhbmRhcmQgZHVtbXkgdGV4dCBldmVyIHNpbmNlIHRoZSAxNTAwcywgd2hlbiBhb"
      "iB1bmtub3duIHByaW50ZXIgdG9vayBhIGdhbGxleSBvZiB0eXBlIGFuZCBzY3JhbWJsZWQg"
      "aXQgdG8gbWFrZSBhIHR5cGUgc3BlY2ltZW4gYm9vay4gSXQgaGFzIHN1cnZpdmVkIG5vdCB"
      "vbmx5IGZpdmUgY2VudHVyaWVzLCBidXQgYWxzbyB0aGUgbGVhcCBpbnRvIGVsZWN0cm9uaW"
      "MgdHlwZXNldHRpbmcsIHJlbWFpbmluZyBlc3NlbnRpYWxseSB1bmNoYW5nZWQuIEl0IHdhc"
      "yBwb3B1bGFyaXNlZCBpbiB0aGUgMTk2MHMgd2l0aCB0aGUgcmVsZWFzZSBvZiBMZXRyYXNl"
      "dCBzaGVldHMgY29udGFpbmluZyBMb3JlbSBJcHN1bSBwYXNzYWdlcywgYW5kIG1vcmUgcmV"
      "jZW50bHkgd2l0aCBkZXNrdG9wIHB1Ymxpc2hpbmcgc29mdHdhcmUgbGlrZSBBbGR1cyBQYW"
      "dlTWFrZXIgaW5jbHVkaW5nIHZlcnNpb25zIG9mIExvcmVtIElwc3VtLg");

  // Create a chunk of binary data.
  constexpr const int BINCHUNK_SIZE = 1024;
  char                binchunk[BINCHUNK_SIZE];
  // Fill the chunk with junk data.
  for (int i = 0; i < BINCHUNK_SIZE; ++i)
    binchunk[i] = (i * 191) % 256;
  for (int i = 0; i < BINCHUNK_SIZE; ++i) {
    auto s = encode(binchunk, i);
    // Make sure the expected encode size is the actual encode size.
    EXPECT_EQUAL(encodeSize(i), s.size());
    // Make sure the expected decode size is the actual debug size.
    EXPECT_EQUAL(decodeSize(s.size()), i);
    auto d = decode(s);
    // Make sure the input and output chunks are the same size.
    EXPECT_EQUAL(d.size(), i);
  }

  return FAIL_COUNTER;
}