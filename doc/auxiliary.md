# Auxiliary functions

## Header

```cpp
#include <tomkv/tom_management.hpp>
```

## tomkv::create_empty_tom function

```cpp
bool create_empty_tom( const std::string& tom_name );
```

If the file with the name `tom_name` is not exists, creates it.

Created file will have the following content:

```xml
<tom><root>
</root></tom>
```

This file can be modified manually or using third-party XML libraries (e.g. `boost/property_tree`) and used in `tomkv::storage` class.

**Returns:** `true` if the file was created, `false` otherwise.

## tomkv::remove_tom function

```cpp
bool remove_tom( const std::string& tom_name );
```

If the file with the name `tom_name` exists, removes this file. Does nothing otherwise.

**Returns:** `true` if the file was removed, `false` otherwise.
