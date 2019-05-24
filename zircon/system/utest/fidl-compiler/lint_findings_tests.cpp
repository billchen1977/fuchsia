// Copyright 2019 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "test_library.h"
#include "unittest_helpers.h"

#include <sstream>

#include <fidl/findings.h>
#include <fidl/template_string.h>
#include <fidl/utils.h>

namespace fidl {

namespace {

#define ASSERT_FINDINGS(TEST) \
    ASSERT_TRUE(test.ExpectFindings(), "Failed")

#define ASSERT_NO_FINDINGS(TEST) \
    ASSERT_TRUE(test.ExpectNoFindings(), "Failed")

class LintTest {

public:
    LintTest() {}

    // Creates a single Finding, clearing any existing Findings first.
    LintTest& ClearFindings() {
        expected_findings_.clear();
        return *this;
    }

    // Adds a Finding to the back of the list of Findings.
    LintTest& AddFinding(std::string check_id, std::string message,
                         std::string violation_string = "${TEST}") {
        assert(!source_template_.str().empty() &&
               "source_template() must be called before AddFinding()");
        std::string template_string = source_template_.str();
        size_t start = template_string.find(violation_string);
        // Note, if there are any substitution variables in the template that
        // preceed the violation_string, the test will probably fail because the
        // string location will probably be different after substitution.
        assert(start != std::string::npos &&
               "Bad test! violation_string not found in template");
        std::string expanded_violation_string =
            TemplateString(violation_string).Substitute(substitutions_);
        auto source_location = library().SourceLocation(
            start, expanded_violation_string.size());
        expected_findings_.emplace_back(
            source_location, check_id, message);
        return *this;
    }

    Finding& GetLastExpectedFinding() {
        if (expected_findings_.empty()) {
            AddFinding(default_check_id_, default_message_);
        }
        return expected_findings_.back();
    }

    LintTest& check_id(std::string check_id) {
        default_check_id_ = check_id;
        return *this;
    }

    LintTest& message(std::string message) {
        default_message_ = message;
        return *this;
    }

    LintTest& suggestion(std::string suggestion) {
        GetLastExpectedFinding().SetSuggestion(suggestion);
        return *this;
    }

    LintTest& replacement(std::string replacement) {
        Finding& finding = GetLastExpectedFinding();
        assert(finding.suggestion().has_value() &&
               "|suggestion| must be added before |replacement|");
        auto description = finding.suggestion()->description();
        finding.SetSuggestion(description, replacement);
        return *this;
    }

    LintTest& source_template(std::string template_str) {
        source_template_ = TemplateString(template_str);
        return *this;
    }

    LintTest& substitute(Substitutions substitutions) {
        substitutions_ = substitutions;
        return *this;
    }

    // Shorthand for the common occurrence of a single substitution variable.
    LintTest& substitute(std::string var_name, std::string value) {
        return substitute({{var_name, value}});
    }

    bool ExpectNoFindings() {
        return execute(/*expect_findings=*/false);
    }

    bool ExpectFindings() {
        return execute(/*expect_findings=*/true);
    }

private:
    bool execute(bool expect_findings) {
        std::ostringstream ss;
        ss << std::endl
           << default_check_id_ << std::endl;
        auto context = (ss.str() + "Bad test!");

        if (expect_findings && expected_findings_.empty()) {
            ASSERT_FALSE(default_message_.empty(), context.c_str());
            AddFinding(default_check_id_, default_message_);
        }

        ASSERT_FALSE((!expect_findings) && (!expected_findings_.empty()),
                     context.c_str());

        ASSERT_TRUE(ValidTest(), context.c_str());

        Findings findings;
        library().Lint(&findings);

        ss << library().source_file().data();
        context = ss.str();

        auto finding = findings.begin();
        auto expected_finding = expected_findings_.begin();

        while (finding != findings.end() &&
               expected_finding != expected_findings_.end()) {
            ASSERT_TRUE(CompareExpectedToActualFinding(*expected_finding, *finding,
                                                       ss.str()));
            expected_finding++;
            finding++;
        }
        if (finding != findings.end()) {
            ss << "\n\n";
            ss << "===================" << std::endl;
            ss << "UNEXPECTED FINDINGS:" << std::endl;
            ss << "===================" << std::endl;
            while (finding != findings.end()) {
                ss << finding->source_location().position() << ": ";
                utils::PrintFinding(ss, *finding);
                ss << std::endl;
                finding++;
            }
            ss << "===================" << std::endl;
            context = ss.str();
            bool has_unexpected_findings = true;
            ASSERT_FALSE(has_unexpected_findings, context.c_str());
        }
        if (expected_finding != expected_findings_.end()) {
            ss << "\n\n";
            ss << "===========================" << std::endl;
            ss << "EXPECTED FINDINGS NOT FOUND:" << std::endl;
            ss << "===========================" << std::endl;
            while (expected_finding != expected_findings_.end()) {
                ss << expected_finding->source_location().position() << ": ";
                utils::PrintFinding(ss, *expected_finding);
                ss << std::endl;
                expected_finding++;
            }
            ss << "===========================" << std::endl;
            context = ss.str();
            bool expected_findings_not_found = true;
            ASSERT_FALSE(expected_findings_not_found, context.c_str());
        }
        // Delete the library. The library owns the |SourceFile| referenced by
        // the |expected_findings_|, so delete those as well.
        library_.reset();
        expected_findings_.clear();
        return true;
    }

    bool ValidTest() const {
        ASSERT_FALSE(source_template_.str().empty(),
                     "Missing source template");
        if (!substitutions_.empty()) {
            ASSERT_FALSE(source_template_.Substitute(substitutions_, false) !=
                             source_template_.Substitute(substitutions_, true),
                         "Missing template substitutions");
        }
        if (expected_findings_.empty()) {
            ASSERT_FALSE(default_check_id_.size() == 0,
                         "Missing check_id");
        } else {
            auto& expected_finding = expected_findings_.front();
            ASSERT_FALSE(expected_finding.subcategory().empty(),
                         "Missing check_id");
            ASSERT_FALSE(expected_finding.message().empty(),
                         "Missing message");
            ASSERT_FALSE(!expected_finding.source_location().valid(),
                         "Missing position");
        }
        return true;
    }

    bool CompareExpectedToActualFinding(const Finding& expectf, const Finding& finding,
                                        std::string test_context) const {
        std::ostringstream ss;
        ss << finding.source_location().position() << ": ";
        utils::PrintFinding(ss, finding);
        auto context = (test_context + ss.str());
        ASSERT_STRING_EQ(expectf.subcategory(),
                         finding.subcategory(),
                         context.c_str());
        ASSERT_STRING_EQ(expectf.source_location().position(),
                         finding.source_location().position(),
                         context.c_str());
        ASSERT_STRING_EQ(expectf.message(),
                         finding.message(),
                         context.c_str());
        ASSERT_EQ(expectf.suggestion().has_value(),
                  finding.suggestion().has_value(),
                  context.c_str());
        if (finding.suggestion().has_value()) {
            ASSERT_STRING_EQ(expectf.suggestion()->description(),
                             finding.suggestion()->description(),
                             context.c_str());
            ASSERT_EQ(expectf.suggestion()->replacement().has_value(),
                      finding.suggestion()->replacement().has_value(),
                      context.c_str());
            if (finding.suggestion()->replacement().has_value()) {
                ASSERT_STRING_EQ(
                    *expectf.suggestion()->replacement(),
                    *finding.suggestion()->replacement(),
                    context.c_str());
            }
        }
        return true;
    }

    static std::string FileLocation(const std::string& within, const std::string& to_find) {
        std::istringstream lines(within);
        std::string line;
        size_t line_number = 0;
        while (std::getline(lines, line)) {
            line_number++;
            size_t column_index = line.find(to_find);
            if (column_index != std::string::npos) {
                std::stringstream position;
                position << "example.fidl:" << line_number << ":" << (column_index + 1);
                return position.str();
            }
        }
        assert(false); // Bug in test
        return "never reached";
    }

    TestLibrary& library() {
        if (!library_) {
            assert(!source_template_.str().empty() &&
                   "source_template() must be set before library() is called");
            library_ = std::make_unique<TestLibrary>(
                source_template_.Substitute(substitutions_));
        }
        return *library_;
    }

    std::string default_check_id_;
    std::string default_message_;
    Findings expected_findings_;
    TemplateString source_template_;
    Substitutions substitutions_;

    std::unique_ptr<TestLibrary> library_;
};

bool constant_repeats_enclosing_type_name() {
    BEGIN_TEST;
    std::map<std::string, std::string> named_templates = {
        {"enum", R"FIDL(
library fidl.repeater;

enum ConstantContainer : int8 {
    ${TEST} = -1;
};
)FIDL"},
        {"bitfield", R"FIDL(
library fidl.repeater;

bits ConstantContainer : uint32 {
  ${TEST} = 0x00000004;
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("name-repeats-enclosing-type-name")
            .source_template(named_template.second);

        test.substitute("TEST", "SOME_VALUE");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "SOME_CONSTANT")
            .message(named_template.first +
                     " member names (constant) must not repeat names from the enclosing " +
                     named_template.first + " 'ConstantContainer'");
        ASSERT_FINDINGS(test);
    }
    END_TEST;
}

bool constant_repeats_library_name() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"constant", R"FIDL(
library fidl.repeater;

const uint64 ${TEST} = 1234;
)FIDL"},
        {"enum member", R"FIDL(
library fidl.repeater;

enum Int8Enum : int8 {
    ${TEST} = -1;
};
)FIDL"},
        {"bitfield member", R"FIDL(
library fidl.repeater;

bits Uint32Bitfield : uint32 {
  ${TEST} = 0x00000004;
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("name-repeats-library-name")
            .source_template(named_template.second);

        test.substitute("TEST", "SOME_CONST");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "LIBRARY_REPEATER")
            .message(named_template.first +
                     " names (repeater) must not repeat names from the library 'fidl.repeater'");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool constant_should_use_common_prefix_suffix_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning for "MINIMUM_..." or "MAXIMUM...", or maybe(?) "..._CAP" Also for instance
    // "SET_CLIENT_NAME_MAX_LEN" -> "MAX_CLIENT_NAME_LEN" or MAX_LEN_CLIENT_NAME", so detect
    // "_MAX" or "_MIN" as separate words in middle or at end of identifier.

    LintTest test;
    test.check_id("constant-should-use-common-prefix-suffix")
        .message("Constants should use the standard prefix and/or suffix for common concept, "
                 "such as MIN and MAX, rather than MINIMUM and MAXIMUM, respectively.")
        .source_template(R"FIDL(
library fidl.a;

const uint64 ${TEST} = 1234;
)FIDL");

    test.substitute("TEST", "MIN_HEIGHT");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "MAX_HEIGHT");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "NAME_MIN_LEN");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "NAME_MAX_LEN");
    ASSERT_NO_FINDINGS(test);

    // Not yet determined if the standard should be LEN or LENGTH, or both
    // test.substitute("TEST", "BYTES_LEN");
    // ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "THRESHOLD_MIN");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "THRESHOLD_MAX");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "MINIMUM_HEIGHT")
        .suggestion("change 'MINIMUM_HEIGHT' to 'MIN_HEIGHT'")
        .replacement("MIN_HEIGHT");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "MAXIMUM_HEIGHT")
        .suggestion("change 'MAXIMUM_HEIGHT' to 'MAX_HEIGHT'")
        .replacement("MAX_HEIGHT");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "NAME_MINIMUM_LEN")
        .suggestion("change 'NAME_MINIMUM_LEN' to 'NAME_MIN_LEN'")
        .replacement("NAME_MIN_LEN");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "NAME_MAXIMUM_LEN")
        .suggestion("change 'NAME_MAXIMUM_LEN' to 'NAME_MAX_LEN'")
        .replacement("NAME_MAX_LEN");
    ASSERT_FINDINGS(test);

    // Not yet determined if the standard should be LEN or LENGTH, or both
    // test.substitute("TEST", "BYTES_LENGTH")
    //     .suggestion("change 'BYTES_LENGTH' to 'BYTES_LEN'")
    //     .replacement("BYTES_LEN");
    // ASSERT_FINDINGS(test);

    test.substitute("TEST", "THRESHOLD_MINIMUM")
        .suggestion("change 'THRESHOLD_MINIMUM' to 'THRESHOLD_MIN'")
        .replacement("THRESHOLD_MIN");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "THRESHOLD_MAXIMUM")
        .suggestion("change 'THRESHOLD_MAXIMUM' to 'THRESHOLD_MAX'")
        .replacement("THRESHOLD_MAX");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "THRESHOLD_CAP")
        .suggestion("change 'THRESHOLD_CAP' to 'THRESHOLD_MAX'")
        .replacement("THRESHOLD_MAX");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool copyright_notice_should_not_flow_through_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("copyright-notice-should-not-flow-through")
        .message("Copyright notice should not flow through")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool decl_member_repeats_enclosing_type_name() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"struct", R"FIDL(
library fidl.repeater;

struct DeclName {
    string ${TEST};
};
)FIDL"},
        {"table", R"FIDL(
library fidl.repeater;

table DeclName {
    1: string ${TEST};
};
)FIDL"},
        {"union", R"FIDL(
library fidl.repeater;

union DeclName {
    string ${TEST};
};
)FIDL"},
        {"xunion", R"FIDL(
library fidl.repeater;

xunion DeclName {
    string ${TEST};
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("name-repeats-enclosing-type-name")
            .source_template(named_template.second);

        test.substitute("TEST", "some_member");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "decl_member")
            .message(named_template.first +
                     " member names (decl) must not repeat names from the enclosing " +
                     named_template.first + " 'DeclName'");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool decl_member_repeats_enclosing_type_name_but_may_disambiguate() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("name-repeats-enclosing-type-name");

    test.source_template(R"FIDL(
library fidl.repeater;

struct SeasonToShirtAndPantMapEntry {
  string season;
  string shirt_type;
  string pant_type;
};
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct SeasonToShirtAndPantMapEntry {
  string season;
  string shirt_and_pant_type;
  bool clashes;
};
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct SeasonToShirtAndPantMapEntry {
  string season;
  string shirt;
  string shirt_for_season;
  bool clashes;
};
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct SeasonToShirtAndPantMapEntry {
  string shirt_color;
  string pant_color;
  bool clashes;
};
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct ShirtAndPantColor {
  string shirt_color;
  string pant_color;
  bool clashes;
};
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

    struct NestedKeyValue {
        string key_key;
        string key_value;
        string value_key;
        string value_value;
    };
)FIDL");

    ASSERT_NO_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct ShirtAndPantSupplies {
  string shirt_color;
  string material;
  string tag;
};
)FIDL")
        .ClearFindings()
        .AddFinding(
            "name-repeats-enclosing-type-name",
            "struct member names (shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_color");

    ASSERT_FINDINGS(test);

    test.source_template(R"FIDL(
    library fidl.repeater;

    struct ShirtAndPantSupplies {
      string shirt_color;
      string shirt_material;
      string tag;
    };
    )FIDL")
        .ClearFindings()
        .AddFinding(
            "name-repeats-enclosing-type-name",
            "struct member names (shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_color")
        .AddFinding(
            "name-repeats-enclosing-type-name",
            "struct member names (shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_material");

    ASSERT_FINDINGS(test);

    test.source_template(R"FIDL(
    library fidl.repeater;

    struct ShirtAndPantSupplies {
      string shirt_and_pant_color;
      string material;
      string shirt_and_pant_tag;
    };
    )FIDL")
        .ClearFindings()
        .AddFinding(
            "name-repeats-enclosing-type-name",
            // repeated words are in lexicographical order; "and" is removed (a stop word)
            "struct member names (pant, shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_and_pant_color")
        .AddFinding(
            "name-repeats-enclosing-type-name",
            // repeated words are in lexicographical order; "and" is removed (a stop word)
            "struct member names (pant, shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_and_pant_tag");

    ASSERT_FINDINGS(test);

    test.source_template(R"FIDL(
library fidl.repeater;

struct ShirtAndPantSupplies {
  string shirt_and_pant_color;
  string material;
  string tag;
};
)FIDL")
        .ClearFindings()
        .AddFinding(
            "name-repeats-enclosing-type-name",
            // repeated words are in lexicographical order; "and" is removed (a stop word)
            "struct member names (pant, shirt) must not repeat names from the enclosing struct 'ShirtAndPantSupplies'",
            "shirt_and_pant_color");

    ASSERT_FINDINGS(test);

    END_TEST;
}

bool decl_member_repeats_library_name() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"struct", R"FIDL(
library fidl.repeater;

struct DeclName {
    string ${TEST};
};
)FIDL"},
        {"table", R"FIDL(
library fidl.repeater;

table DeclName {
    1: string ${TEST};
};
)FIDL"},
        {"union", R"FIDL(
library fidl.repeater;

union DeclName {
    string ${TEST};
};
)FIDL"},
        {"xunion", R"FIDL(
library fidl.repeater;

xunion DeclName {
    string ${TEST};
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("name-repeats-library-name")
            .source_template(named_template.second);

        test.substitute("TEST", "some_member");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "library_repeater")
            .message(named_template.first +
                     " member names (repeater) must not repeat names from the library "
                     "'fidl.repeater'");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool decl_name_repeats_library_name() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"protocol", R"FIDL(
library fidl.repeater;

protocol ${TEST} {};
)FIDL"},
        {"method", R"FIDL(
library fidl.repeater;

protocol TestProtocol {
    ${TEST}();
};
)FIDL"},
        {"enum", R"FIDL(
library fidl.repeater;

enum ${TEST} : int8 {
    SOME_CONST = -1;
};
)FIDL"},
        {"bitfield", R"FIDL(
library fidl.repeater;

bits ${TEST} : uint32 {
  SOME_BIT = 0x00000004;
};
)FIDL"},
        {"struct", R"FIDL(
library fidl.repeater;

struct ${TEST} {
    string decl_member;
};
)FIDL"},
        {"table", R"FIDL(
library fidl.repeater;

table ${TEST} {
    1: string decl_member;
};
)FIDL"},
        {"union", R"FIDL(
library fidl.repeater;

union ${TEST} {
    string decl_member;
};
)FIDL"},
        {"xunion", R"FIDL(
library fidl.repeater;

xunion ${TEST} {
    string decl_member;
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("name-repeats-library-name")
            .source_template(named_template.second);

        test.substitute("TEST", "UrlLoader");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "LibraryRepeater")
            .message(named_template.first + " names (repeater) must not repeat names from the library 'fidl.repeater'");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool deprecation_comment_should_not_flow_through_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on DEPRECATED comments.

    LintTest test;
    test.check_id("deprecation-comment-should-not-flow-through")
        .message("DEPRECATED comments should not flow through")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool disallowed_library_name_component() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("disallowed-library-name-component")
        .message("Library names must not contain the following components: common, service, util, "
                 "base, f<letter>l, zx<word>")
        .source_template(R"FIDL(
library fidl.${TEST};
)FIDL");

    test.substitute("TEST", "display");
    ASSERT_NO_FINDINGS(test);

    // Bad test: zx<word>
    test.substitute("TEST", "zxsocket");
    // no suggestion
    ASSERT_FINDINGS(test);

    // Bad test: f<letter>l
    test.substitute("TEST", "ful");
    // no suggestion
    ASSERT_FINDINGS(test);

    // Bad test: banned words like "common"
    test.substitute("TEST", "common");
    // no suggestion
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool protocol_name_ends_in_service_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Error if ends in "Service", warning if includes "Service" as a word, but "Serviceability"
    // ("Service" is only part of a word) is OK.

    LintTest test;
    test.check_id("protocol-name-ends-in-service")
        .message("Protocols must not include the name 'service.'")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool protocol_name_includes_service_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Error if ends in "Service", warning if includes "Service" as a word, but "Serviceability"
    // ("Service" is only part of a word) is OK.

    LintTest test;
    test.check_id("protocol-name-includes-service")
        .message("Protocols must not include the name 'service.'")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool discontiguous_comment_block_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("discontiguous-comment-block")
        .message("Multi-line comments should be in a single contiguous comment block, with all "
                 "lines prefixed by comment markers")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool end_of_file_should_be_one_newline_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Use formatter.
    //
    // Warning: often hard to get right depending on the editor.

    LintTest test;
    test.check_id("end-of-file-should-be-one-newline")
        .message("End files with exactly one newline character")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool event_names_must_start_with_on() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("event-names-must-start-with-on")
        .message("Event names must start with 'On'")
        .source_template(R"FIDL(
library fidl.a;

protocol TestProtocol {
  -> ${TEST}();
};
)FIDL");

    test.substitute("TEST", "OnPress");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "Press")
        .suggestion("change 'Press' to 'OnPress'")
        .replacement("OnPress");
    ASSERT_FINDINGS(test);

    test.substitute("TEST", "OntologyUpdate")
        .suggestion("change 'OntologyUpdate' to 'OnOntologyUpdate'")
        .replacement("OnOntologyUpdate");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool excessive_number_of_separate_protocols_for_file_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning(?) if a fidl file contains more than some tolerance cap number of protocols.
    //
    // Or if a directory of fidl files contains more than some tolerance number of files AND any
    // fidl file(s) in that directory contains more than some smaller cap number of protocols per
    // fidl file. The fuchsia.ledger would be a good one to look at since it defines many protocols.
    // We do not have public vs private visibility yet, and the cap may only be needed for public
    // things.

    LintTest test;
    test.check_id("excessive-number-of-separate-protocols-for-file")
        .message("Some libraries create separate protocol instances for every logical object in "
                 "the protocol, but this approach has a number of disadvantages:")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool excessive_number_of_separate_protocols_for_library_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Or if a directory of fidl files contains more than some tolerance number of files AND any
    // fidl file(s) in that directory contains more than some smaller cap number of protocols per
    // fidl file. The fuchsia.ledger would be a good one to look at since it defines many protocols.
    // We do not have public vs private visibility yet, and the cap may only be needed for public
    // things.

    LintTest test;
    test.check_id("excessive-number-of-separate-protocols-for-library")
        .message("Some libraries create separate protocol instances for every logical object in "
                 "the protocol, but this approach has a number of disadvantages:")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool inconsistent_type_for_recurring_file_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("inconsistent-type-for-recurring-file-concept")
        .message("Use consistent types for the same concept")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool inconsistent_type_for_recurring_library_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("inconsistent-type-for-recurring-library-concept")
        .message("Ideally, types would be used consistently across library boundaries")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool incorrect_line_indent_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Use formatter: It can handle all checks in this section ("Syntax"), but linter can run the
    // formatter and diff results to see if the file needs to be formatted.

    LintTest test;
    test.check_id("incorrect-line-indent")
        .message("Use 4 space indents")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool incorrect_spacing_between_declarations_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Use formatter.

    LintTest test;
    test.check_id("incorrect-spacing-between-declarations")
        .message("Separate declarations for struct, union, enum, and protocol constructs from "
                 "other declarations with one blank line (two consecutive newline characters)")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool invalid_case_for_constant() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"constants", R"FIDL(
library fidl.a;

const uint64 ${TEST} = 1234;
)FIDL"},
        {"enum members", R"FIDL(
library fidl.a;

enum Int8Enum : int8 {
    ${TEST} = -1;
};
)FIDL"},
        {"bitfield members", R"FIDL(
library fidl.a;

bits Uint32Bitfield : uint32 {
  ${TEST} = 0x00000004;
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("invalid-case-for-constant")
            .message(named_template.first + " must be named in ALL_CAPS_SNAKE_CASE")
            .source_template(named_template.second);

        test.substitute("TEST", "SOME_CONST");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "some_CONST")
            .suggestion("change 'some_CONST' to 'SOME_CONST'")
            .replacement("SOME_CONST");
        ASSERT_FINDINGS(test);

        test.substitute("TEST", "kSomeConst")
            .suggestion("change 'kSomeConst' to 'SOME_CONST'")
            .replacement("SOME_CONST");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool invalid_case_for_decl_member() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"parameters", R"FIDL(
library fidl.a;

protocol TestProtocol {
    SomeMethod(string ${TEST});
};
)FIDL"},
        {"struct members", R"FIDL(
library fidl.a;

struct DeclName {
    string ${TEST};
};
)FIDL"},
        {"table members", R"FIDL(
library fidl.a;

table DeclName {
    1: string ${TEST};
};
)FIDL"},
        {"union members", R"FIDL(
library fidl.a;

union DeclName {
    string ${TEST};
};
)FIDL"},
        {"xunion members", R"FIDL(
library fidl.a;

xunion DeclName {
    string ${TEST};
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("invalid-case-for-decl-member")
            .message(named_template.first + " must be named in lower_snake_case")
            .source_template(named_template.second);

        test.substitute("TEST", "agent_request_count");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "agentRequestCount")
            .suggestion("change 'agentRequestCount' to 'agent_request_count'")
            .replacement("agent_request_count");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool invalid_case_for_decl_name() {
    BEGIN_TEST;

    std::map<std::string, std::string> named_templates = {
        {"protocols", R"FIDL(
library fidl.a;

protocol ${TEST} {};
)FIDL"},
        {"methods", R"FIDL(
library fidl.a;

protocol TestProtocol {
  ${TEST}();
};
)FIDL"},
        {"enums", R"FIDL(
library fidl.a;

enum ${TEST} : int8 {
    SOME_CONST = -1;
};
)FIDL"},
        {"bitfields", R"FIDL(
library fidl.a;

bits ${TEST} : uint32 {
  SOME_BIT = 0x00000004;
};
)FIDL"},
        {"structs", R"FIDL(
library fidl.a;

struct ${TEST} {
    string decl_member;
};
)FIDL"},
        {"tables", R"FIDL(
library fidl.a;

table ${TEST} {
    1: string decl_member;
};
)FIDL"},
        {"unions", R"FIDL(
library fidl.a;

union ${TEST} {
    string decl_member;
};
)FIDL"},
        {"xunions", R"FIDL(
library fidl.a;

xunion ${TEST} {
    string decl_member;
};
)FIDL"},
    };

    for (auto const& named_template : named_templates) {
        LintTest test;
        test.check_id("invalid-case-for-decl-name")
            .message(named_template.first + " must be named in UpperCamelCase")
            .source_template(named_template.second);

        test.substitute("TEST", "UrlLoader");
        ASSERT_NO_FINDINGS(test);

        test.substitute("TEST", "URLLoader")
            .suggestion("change 'URLLoader' to 'UrlLoader'")
            .replacement("UrlLoader");
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool invalid_case_for_decl_name_for_event() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("invalid-case-for-decl-name")
        .message("events must be named in UpperCamelCase")
        .source_template(R"FIDL(
library fidl.a;

protocol TestProtocol {
  -> ${TEST}();
};
)FIDL");

    test.substitute("TEST", "OnUrlLoader");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "OnURLLoader")
        .suggestion("change 'OnURLLoader' to 'OnUrlLoader'")
        .replacement("OnUrlLoader");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool invalid_case_for_primitive_alias() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("invalid-case-for-primitive-alias")
        .message("Primitive aliases must be named in lower_snake_case")
        .source_template(R"FIDL(
library fidl.a;

using foo as ${TEST};
using bar as baz;
)FIDL");

    test.substitute("TEST", "what_if_someone_does_this");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "WhatIfSomeoneDoes_This")
        .suggestion("change 'WhatIfSomeoneDoes_This' to 'what_if_someone_does_this'")
        .replacement("what_if_someone_does_this");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool invalid_copyright_for_platform_source_library_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("invalid-copyright-for-platform-source-library")
        .message("FIDL files defined in the Platform Source Tree (i.e., defined in "
                 "fuchsia.googlesource.com) must begin with the standard copyright notice")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool library_name_does_not_match_file_path_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("library-name-does-not-match-file-path")
        .message("The <library> directory is named using the dot-separated name of the FIDL "
                 "library")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool manager_protocols_are_discouraged_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("manager-protocols-are-discouraged")
        .message("The name Manager may be used as a name of last resort for a protocol with broad "
                 "scope")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool method_repeats_enclosing_type_name() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("name-repeats-enclosing-type-name")
        .source_template(R"FIDL(
library fidl.repeater;

protocol TestProtocol {
    ${TEST}();
};
)FIDL");

    test.substitute("TEST", "SomeMethod");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "ProtocolMethod")
        .message("method names (protocol) must not repeat names from the enclosing protocol "
                 "'TestProtocol'");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool method_return_status_missing_ok_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning or error(?) if returning a "status" enum that does not have an OK value. Note there
    // will be (or is) new guidance here.
    //
    // From the rubric:
    //
    //   If a method can return either an error or a result, use the following pattern:
    //
    //     enum MyStatus { OK; FOO; BAR; ... };
    //
    //     protocol Frobinator {
    //         1: Frobinate(...) -> (MyStatus status, FrobinateResult? result);
    //     };

    LintTest test;
    test.check_id("method-return-status-missing-ok")
        .message("") // TBD
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool method_returns_status_with_non_optional_result_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning if return a status and a non-optional result? we now have another more expressive
    // pattern for this, this section should be updated. Specifically, see:
    // https://fuchsia.googlesource.com/fuchsia/+/master/docs/development/languages/fidl/reference/ftp/ftp-014.md.

    LintTest test;
    test.check_id("method-returns-status-with-non-optional-result")
        .message("") // TBD
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool method_should_pipeline_protocols_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Error(?) if the return tuple contains one value of another FIDL protocol type. Returning a
    // protocol is better done by sending a request for pipelining. This will be hard to lint at the
    // raw level, because you do not know to differentiate Bar from a protocol vs a message vs a bad
    // name since resolution is done later. This may call for linting to be done on the JSON IR.

    LintTest test;
    test.check_id("method-should-pipeline-protocols")
        .message("") // TBD
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool no_commonly_reserved_words_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("no-commonly-reserved-words")
        .message("Avoid commonly reserved words")
        .source_template(R"FIDL(
library fidl.a;

using foo as ${TEST};
)FIDL");

    // Unique union of reserved words from:
    // FIDL, C++, Rust, Dart, Go, Java, JavaScript, and TypeScript
    auto checked_words = {
        "_",
        "abstract",
        "and",
        "and_eq",
        "any",
        "array",
        "as",
        "asm",
        "assert",
        "async",
        "auto",
        "await",
        "become",
        "bitand",
        "bitor",
        "bits",
        "bool",
        "boolean",
        "box",
        "break",
        "byte",
        "case",
        "catch",
        "chan",
        "char",
        "class",
        "compl",
        "const",
        "const_cast",
        "constructor",
        "continue",
        "covariant",
        "crate",
        "debugger",
        "declare",
        "default",
        "defer",
        "deferred",
        "delete",
        "do",
        "double",
        "dyn",
        "dynamic",
        "dynamic_cast",
        "else",
        "enum",
        "error",
        "explicit",
        "export",
        "extends",
        "extern",
        "external",
        "factory",
        "fallthrough",
        "false",
        "final",
        "finally",
        "float",
        "fn",
        "for",
        "friend",
        "from",
        "func",
        "function",
        "get",
        "go",
        "goto",
        "handle",
        "hide",
        "if",
        "impl",
        "implements",
        "import",
        "in",
        "inline",
        "instanceof",
        "int",
        "interface",
        "is",
        "let",
        "library",
        "long",
        "loop",
        "macro",
        "map",
        "match",
        "mixin",
        "mod",
        "module",
        "move",
        "mut",
        "mutable",
        "namespace",
        "native",
        "new",
        "not",
        "not_eq",
        "null",
        "number",
        "of",
        "on",
        "operator",
        "or",
        "or_eq",
        "override",
        "package",
        "part",
        "priv",
        "private",
        "protected",
        "protocol",
        "pub",
        "public",
        "range",
        "ref",
        "register",
        "reinterpret_cast",
        "request",
        "require",
        "reserved",
        "rethrow",
        "return",
        "select",
        "self",
        "set",
        "short",
        "show",
        "signed",
        "sizeof",
        "static",
        "static_cast",
        "strictfp",
        "string",
        "struct",
        "super",
        "switch",
        "symbol",
        "sync",
        "synchronized",
        "table",
        "template",
        "this",
        "throw",
        "throws",
        "trait",
        "transient",
        "true",
        "try",
        "type",
        "typedef",
        "typeid",
        "typename",
        "typeof",
        "union",
        "unsafe",
        "unsigned",
        "unsized",
        "use",
        "using",
        "var",
        "vector",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "where",
        "while",
        "with",
        "xor",
        "xor_eq",
        "xunion",
        "yield",
    };

    for (auto word : checked_words) {
        test.substitute("TEST", word);
        ASSERT_FINDINGS(test);
    }

    END_TEST;
}

bool note_comment_should_not_flow_through_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on NOTE comments.

    LintTest test;
    test.check_id("note-comment-should-not-flow-through")
        .message("NOTE comments should not flow through")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool primitive_alias_repeats_library_name() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("name-repeats-library-name")
        .source_template(R"FIDL(
library fidl.repeater;

using uint64 as ${TEST};
)FIDL");

    test.substitute("TEST", "some_alias");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "library_repeater")
        .message("primitive alias names (repeater) must not repeat names from the library "
                 "'fidl.repeater'");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool service_hub_pattern_is_discouraged_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning(?) Note this is a low-priority check.

    LintTest test;
    test.check_id("service-hub-pattern-is-discouraged")
        .message("") // TBD
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool string_bounds_not_specified_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("string-bounds-not-specified")
        .message("Specify bounds for string")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool tabs_disallowed_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Use formatter.

    LintTest test;
    test.check_id("tabs-disallowed")
        .message("Never use tabs")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool todo_comment_should_not_flow_through_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on TODO comments.

    LintTest test;
    test.check_id("todo-comment-should-not-flow-through")
        .message("TODO comments should not flow through")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool too_many_nested_libraries_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("too-many-nested-libraries")
        .message("Avoid library names with more than two dots")
        .source_template(R"FIDL(
library ${TEST};
)FIDL");

    test.substitute("TEST", "fidl.a");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "fuchsia.foo.bar.baz");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool trailing_whitespace_disallowed_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Use formatter.

    LintTest test;
    test.check_id("trailing-whitespace-disallowed")
        .message("Avoid trailing whitespace")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool unexpected_type_for_well_known_buffer_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on struct, union, and table member name patterns.

    LintTest test;
    test.check_id("unexpected-type-for-well-known-buffer-concept")
        .message("Use fuchsia.mem.Buffer for images and (large) protobufs, when it makes sense to "
                 "buffer the data completely")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool unexpected_type_for_well_known_bytes_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // (two suggestions) recommend either bytes or array<uint8>. warning on struct, union, and table
    // member name patterns.

    LintTest test;
    test.check_id("unexpected-type-for-well-known-bytes-concept")
        .message("Use bytes or array<uint8> for small non-text data:")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool unexpected_type_for_well_known_socket_handle_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on struct, union, and table member name patterns.

    LintTest test;
    test.check_id("unexpected-type-for-well-known-socket-handle-concept")
        .message("Use handle<socket> for audio and video streams because data may arrive over "
                 "time, or when it makes sense to process data before completely written or "
                 "available")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool unexpected_type_for_well_known_string_concept_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    // Warning on struct, union, and table members that include certain well-known concepts (like
    // "filename" and "file_name") but their types don't match the type recommended (e.g., string,
    // in this case).

    LintTest test;
    test.check_id("unexpected-type-for-well-known-string-concept")
        .message("Use string for text data:")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool vector_bounds_not_specified_please_implement_me() {
    if (true)
        return true; // disabled pending feature implementation
    BEGIN_TEST;

    LintTest test;
    test.check_id("vector-bounds-not-specified")
        .message("Specify bounds for vector")
        .source_template(R"FIDL(
library fidl.a;

PUT FIDL CONTENT HERE WITH PLACEHOLDERS LIKE:
    ${TEST}
TO SUBSTITUTE WITH GOOD_VALUE AND BAD_VALUE CASES.
)FIDL");

    test.substitute("TEST", "!GOOD_VALUE!");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "!BAD_VALUE!")
        .suggestion("change '!BAD_VALUE!' to '!GOOD_VALUE!'")
        .replacement("!GOOD_VALUE!");
    ASSERT_FINDINGS(test);

    END_TEST;
}

bool wrong_prefix_for_platform_source_library() {
    BEGIN_TEST;

    LintTest test;
    test.check_id("wrong-prefix-for-platform-source-library")
        .message("FIDL library name is not currently allowed")
        .source_template(R"FIDL(
library ${TEST}.subcomponent;
)FIDL");

    test.substitute("TEST", "fuchsia");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "fidl");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "test");
    ASSERT_NO_FINDINGS(test);

    test.substitute("TEST", "mylibs")
        .suggestion("change 'mylibs' to fuchsia, perhaps?")
        .replacement("fuchsia, perhaps?");
    ASSERT_FINDINGS(test);

    END_TEST;
}

BEGIN_TEST_CASE(lint_findings_tests)

RUN_TEST(constant_repeats_library_name)
RUN_TEST(constant_repeats_enclosing_type_name)
RUN_TEST(constant_should_use_common_prefix_suffix_please_implement_me)
RUN_TEST(copyright_notice_should_not_flow_through_please_implement_me)
RUN_TEST(decl_member_repeats_enclosing_type_name)
RUN_TEST(decl_member_repeats_enclosing_type_name_but_may_disambiguate)
RUN_TEST(decl_member_repeats_library_name)
RUN_TEST(decl_name_repeats_library_name)
RUN_TEST(deprecation_comment_should_not_flow_through_please_implement_me)
RUN_TEST(disallowed_library_name_component)
RUN_TEST(discontiguous_comment_block_please_implement_me)
RUN_TEST(end_of_file_should_be_one_newline_please_implement_me)
RUN_TEST(event_names_must_start_with_on)
RUN_TEST(excessive_number_of_separate_protocols_for_file_please_implement_me)
RUN_TEST(excessive_number_of_separate_protocols_for_library_please_implement_me)
RUN_TEST(inconsistent_type_for_recurring_file_concept_please_implement_me)
RUN_TEST(inconsistent_type_for_recurring_library_concept_please_implement_me)
RUN_TEST(incorrect_line_indent_please_implement_me)
RUN_TEST(incorrect_spacing_between_declarations_please_implement_me)
RUN_TEST(invalid_case_for_constant)
RUN_TEST(invalid_case_for_decl_member)
RUN_TEST(invalid_case_for_decl_name)
RUN_TEST(invalid_case_for_decl_name_for_event)
RUN_TEST(invalid_case_for_primitive_alias)
RUN_TEST(invalid_copyright_for_platform_source_library_please_implement_me)
RUN_TEST(library_name_does_not_match_file_path_please_implement_me)
RUN_TEST(manager_protocols_are_discouraged_please_implement_me)
RUN_TEST(method_repeats_enclosing_type_name)
RUN_TEST(method_return_status_missing_ok_please_implement_me)
RUN_TEST(method_returns_status_with_non_optional_result_please_implement_me)
RUN_TEST(method_should_pipeline_protocols_please_implement_me)
RUN_TEST(no_commonly_reserved_words_please_implement_me)
RUN_TEST(note_comment_should_not_flow_through_please_implement_me)
RUN_TEST(primitive_alias_repeats_library_name)
RUN_TEST(protocol_name_ends_in_service_please_implement_me)
RUN_TEST(protocol_name_includes_service_please_implement_me)
RUN_TEST(service_hub_pattern_is_discouraged_please_implement_me)
RUN_TEST(string_bounds_not_specified_please_implement_me)
RUN_TEST(tabs_disallowed_please_implement_me)
RUN_TEST(todo_comment_should_not_flow_through_please_implement_me)
RUN_TEST(too_many_nested_libraries_please_implement_me)
RUN_TEST(trailing_whitespace_disallowed_please_implement_me)
RUN_TEST(unexpected_type_for_well_known_buffer_concept_please_implement_me)
RUN_TEST(unexpected_type_for_well_known_bytes_concept_please_implement_me)
RUN_TEST(unexpected_type_for_well_known_socket_handle_concept_please_implement_me)
RUN_TEST(unexpected_type_for_well_known_string_concept_please_implement_me)
RUN_TEST(vector_bounds_not_specified_please_implement_me)
RUN_TEST(wrong_prefix_for_platform_source_library)

END_TEST_CASE(lint_findings_tests)

} // namespace

} // namespace fidl
