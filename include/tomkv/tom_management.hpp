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

#ifndef __TOMKV_INCLUDE_TOM_MANAGEMENT_HPP
#define __TOMKV_INCLUDE_TOM_MANAGEMENT_HPP

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/xml_parser.hpp"
#include "boost/filesystem.hpp"
#include <string>

namespace tomkv {
namespace internal {

namespace pt = boost::property_tree;
namespace fs = boost::filesystem;

bool create_empty_tom( const std::string& tom_name ) {
    if (!fs::exists(tom_name)) {
        pt::ptree tree;

        tree.add("tom.root", "");

        pt::write_xml(tom_name, tree);
        return true;
    }
    return false;
}

bool remove_tom( const std::string& tom_name ) {
    if (fs::exists(tom_name)) {
        fs::remove(tom_name);
        return true;
    }
    return false;
}

} // namespace internal

using internal::create_empty_tom;
using internal::remove_tom;

} // namespace tomkv

#endif // __TOMKV_INCLUDE_TOM_MANAGEMENT_HPP
