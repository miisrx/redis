# 9: Data Serialization

What if we want to return more complicated data?

**Serialization:** encoding to handle different types of data

We will use the serialization “type-length-value” (TLV): 
- “Type” indicates the type of the value
- “Length” is for variable length data such as strings or arrays
- “Value” is the encoded at last

it has many advantages:
- can be decoded without a schema (like JSON, XML)
- can encode arbitrarily nested data


