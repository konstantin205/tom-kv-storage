/*
 * MIT License
 *
 * Copyright (c) 2020 Konstantin Boyarinov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include <tomkv/tom_management.hpp>
#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include <filesystem>

namespace pt = boost::property_tree;

TEST_CASE("test create tom") {
    std::string tom_name = "tom.xml";
    REQUIRE_MESSAGE(!std::filesystem::exists(tom_name), "Incorrect test setup");

    bool created = tomkv::create_empty_tom(tom_name);
    REQUIRE_MESSAGE(created, "Empty tom should be created");

    created = tomkv::create_empty_tom(tom_name);
    REQUIRE_MESSAGE(!created, "Duplicated empty tom should not be created");

    pt::ptree tree;
    pt::read_xml(tom_name, tree);

    auto child = tree.get_child("tom.root");
    REQUIRE_MESSAGE(child.empty(), "Tom should be empty");
    std::filesystem::remove(tom_name);
}

TEST_CASE("test remove tom") {
    std::string tom_name = "tom.xml";
    REQUIRE_MESSAGE(!std::filesystem::exists(tom_name), "Incorrect test setup");

    bool removed = tomkv::remove_tom(tom_name);
    REQUIRE_MESSAGE(!removed, "tom is not exists - it should not be removed");

    // Create a tom using boost
    pt::ptree tree;
    tree.add("tom.root", "");

    pt::write_xml(tom_name, tree);

    removed = tomkv::remove_tom(tom_name);
    REQUIRE_MESSAGE(removed, "Existing tom should be removed");
    REQUIRE_MESSAGE(!std::filesystem::exists(tom_name), "Tom was not actually removed");
}