

import enum
import os
import pathlib
import re

class EncodeType(enum.IntEnum):
	Text = 0
	Enum = 1
	Dict = 2
	Switch = 3

__folder__ = str(pathlib.Path(__file__).parent)
ERR_DEFINE_RE = re.compile(r"\s*#\s*define\s+(\w+)\s+(\d+)")
STRING_PREFIX = ''

with open(__folder__ + "/errno.txt", 'r') as f:
	text = f.read()


print("valid options are:")
print("  'switch' / 'Dict' / 'enum' / ['text']")

encode_type_str = input("encode type: ").strip()
encode_type_str = encode_type_str.lower()

encode_type: EncodeType = EncodeType.Text 

if encode_type_str == 'enum':
	encode_type = EncodeType.Enum
elif encode_type_str == 'dict':
	encode_type = EncodeType.Dict
elif encode_type_str == 'switch':
	encode_type = EncodeType.Switch

with open(__folder__ + "/errno.csv", 'w') as f:
	for i in ERR_DEFINE_RE.finditer(text):
		if encode_type == EncodeType.Text:
			f.write(f"{STRING_PREFIX}\"{i[1]}\",\n")
		elif encode_type == EncodeType.Enum:
			f.write(f"{i[1]} = {i[2]},\n")
		elif encode_type == EncodeType.Dict:
			f.write('{')
			f.write(f'{i[2]}, {STRING_PREFIX}\"{i[1]}\"')
			f.write('},\n')
		elif encode_type == EncodeType.Switch:
			f.write(f'case {i[1]}: \n\treturn {STRING_PREFIX}\"{i[1]}\";\n')
