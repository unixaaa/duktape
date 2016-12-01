==============
Duktape typing
==============

Internal and external type summary
==================================

The table below summarizes various types and how they appear in the external
API and internals:

+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| Ecmascript code            | API type            | API type check(s)      | Internal duk_tval tag | Internal struct(s)      |                                     | Ecmascript typeof | Ecmascript Object .toString() | Notes                             |
+============================+=====================+========================+=======================+=========================+===================+===============================+===================================+
| n/a                        | DUK_TYPE_NONE       | duk_is_valid_index()   | DUK_TAG_UNUSED        | n/a                     |                                     | n/a               | n/a                           | Marker for "no value" when doing  |
|                            |                     |                        |                       |                         |                                     |                   |                               | a valus stack type lookup.        |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| void 0                     | DUK_TYPE_UNDEFINED  | duk_is_undefined()     | DUK_TAG_UNDEFINED     | n/a                     |                                     | undefined         | [object Undefined]            |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| null                       | DUK_TYPE_NULL       | duk_is_null()          | DUK_TAG_NULL          | n/a                     |                                     | object (!)        | [object Null]                 |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| true                       | DUK_TYPE_BOOLEAN    | duk_is_boolean()       | DUK_TAG_BOOLEAN       | n/a                     |                                     | boolean           | [object Boolean]              |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| 123                        | DUK_TYPE_NUMBER     | duk_is_number()        | DUK_TAG_FASTINT       | n/a                     |                                     | number            | [object Number]               | If 48-bit signed int, and fastint |
|                            |                     |                        |                       |                         |                                     |                   |                               | support enabled.                  |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| 123.1                      | DUK_TYPE_NUMBER     | duk_is_number()        | DUK_TAG_NUMBER (*)    | n/a                     |                                     | number            | [object Number]               | With packed duk_tval, no explicit |
|                            |                     |                        |                       |                         |                                     |                   |                               | internal tag.                     |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| "foo"                      | DUK_TYPE_STRING     | duk_is_string()        | DUK_TAG_STRING        | duk_hstring             |                                     | string            | [object String]               |                                   |
|                            |                     |                        |                       | duk_hstring_external    |                                     |                   |                               |                                   | 
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| Symbol('foo')              | DUK_TYPE_STRING     | duk_is_string()        | DUK_TAG_STRING        | duk_hstring             |                                     | symbol            | [object Symbol]               | Symbols                           |
|                            |                     |                        |                       | duk_hstring_external    |                                     |                   |                               | (NOT FINALIZED)                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| n/a                        | DUK_TYPE_LIGHTFUNC  | duk_is_lightfunc()     | DUK_TAG_LIGHTFUNC     | n/a                     |                                     | function          | [object Function]             |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| ArrayBuffer.allocPlain(1)  | DUK_TYPE_BUFFER     | duk_is_buffer()        | DUK_TAG_BUFFER        | duk_hbuffer_fixed       |                                     | object            | [object ArrayBuffer]          |                                   |
|                            |                     |                        |                       | duk_hbuffer_dynamic     |                                     |                   |                               |                                   |
|                            |                     |                        |                       | duk_hbuffer_external    |                                     |                   |                               |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| Duktape.Pointer('dummy')   | DUK_TYPE_POINTER    | duk_is_pointer()       | DUK_TAG_POINTER       | n/a                     | n/a                                 | pointer           | [object Pointer]              |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| n/a                        | n/a                 | duk_is_valid_index()   | n/a                   | n/a                     |                                     | n/a               | n/a                           | Marker for "no value" when doing  |
|                            |                     |                        |                       |                         |                                     |                   |                               | a class number lookup.            |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| { foo: 123 }               | DUK_TYPE_OBJECT     | duk_is_object()        | DUK_TAG_OBJECT        | duk_hobject             |                                     | object            | [object Object]               |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| [ 1, 2, 3 ]                | DUK_TYPE_OBJECT     | duk_is_object()        | DUK_TAG_OBJECT        | duk_harray              |                                     | object            | [object Array]                | duk_harray extends duk_hobject.   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+
| new Uint8ClampedArray(1)   | DUK_TYPE_OBJECT     | duk_is_object()        | DUK_TAG_OBJECT        | duk_hbufobj             | DUK_HOBJECT_CLASS_UINT8CLAMPEDARRAY | object            | [object Uint8ClampedArray     |                                   |
+----------------------------+---------------------+------------------------+-----------------------+-------------------------+-------------------------------------+-------------------+-------------------------------+-----------------------------------+

TBD: all remaining subclasses (duk_hnativefunc, etc)::

    #define DUK_HOBJECT_CLASS_NONE                 0
    #define DUK_HOBJECT_CLASS_ARGUMENTS            1
    #define DUK_HOBJECT_CLASS_ARRAY                2
    #define DUK_HOBJECT_CLASS_BOOLEAN              3
    #define DUK_HOBJECT_CLASS_DATE                 4
    #define DUK_HOBJECT_CLASS_ERROR                5
    #define DUK_HOBJECT_CLASS_FUNCTION             6
    #define DUK_HOBJECT_CLASS_JSON                 7
    #define DUK_HOBJECT_CLASS_MATH                 8
    #define DUK_HOBJECT_CLASS_NUMBER               9
    #define DUK_HOBJECT_CLASS_OBJECT               10
    #define DUK_HOBJECT_CLASS_REGEXP               11
    #define DUK_HOBJECT_CLASS_STRING               12
    #define DUK_HOBJECT_CLASS_GLOBAL               13
    #define DUK_HOBJECT_CLASS_OBJENV               14  /* custom */
    #define DUK_HOBJECT_CLASS_DECENV               15  /* custom */
    #define DUK_HOBJECT_CLASS_POINTER              16  /* custom */
    #define DUK_HOBJECT_CLASS_THREAD               17  /* custom; implies DUK_HOBJECT_IS_THREAD */
    #define DUK_HOBJECT_CLASS_BUFOBJ_MIN           18
    #define DUK_HOBJECT_CLASS_ARRAYBUFFER          18  /* implies DUK_HOBJECT_IS_BUFOBJ */
    #define DUK_HOBJECT_CLASS_DATAVIEW             19
    #define DUK_HOBJECT_CLASS_INT8ARRAY            20
    #define DUK_HOBJECT_CLASS_UINT8ARRAY           21
    #define DUK_HOBJECT_CLASS_UINT8CLAMPEDARRAY    22
    #define DUK_HOBJECT_CLASS_INT16ARRAY           23
    #define DUK_HOBJECT_CLASS_UINT16ARRAY          24
    #define DUK_HOBJECT_CLASS_INT32ARRAY           25
    #define DUK_HOBJECT_CLASS_UINT32ARRAY          26
    #define DUK_HOBJECT_CLASS_FLOAT32ARRAY         27
    #define DUK_HOBJECT_CLASS_FLOAT64ARRAY         28
    #define DUK_HOBJECT_CLASS_BUFOBJ_MAX           28
    #define DUK_HOBJECT_CLASS_MAX                  28
