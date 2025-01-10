#  bekOS is a basic OS for the Raspberry Pi
#  Copyright (C) 2025 Bekos Contributors
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.
import argparse
import dataclasses
from pathlib import Path
import xml.etree.ElementTree as ET


def to_camel(s: str):
    return s.replace("_", " ").title().replace(" ", "")


@dataclasses.dataclass
class Message:
    name: str
    arguments: list[tuple[str, str]]
    kind: str
    response: "Message | None" = None
    number: int = None


@dataclasses.dataclass
class Interface:
    name: str
    namespace: str | None
    includes: list[str]
    requests: list[Message]
    events: list[Message]
    types: dict[str, str]


def parse_file(tree: ET.ElementTree, base_name: str):
    root = tree.getroot()
    namespace = root.get("namespace")
    requests = []
    events = []
    types = {}
    includes = []
    for child in root:
        name = child.get("name")
        if not name and child.tag != "include":
            raise RuntimeError("Element must have a name!")
        if child.tag == "request" or child.tag == "event":
            # First, find arguments
            args = []
            for arg in child.iter("arg"):
                if not arg.get("name"):
                    raise RuntimeError("Argument must have name")
                args.append((arg.get("name"), arg.get("type")))
            is_async = child.get("type", "async") == "async"
            msg = Message(name, args, "async" if is_async else "sync")
            if (resp := child.find("response")) is not None:
                if is_async:
                    raise RuntimeError("Asynchronous Message cannot have response!")
                response_args = []
                for arg in resp.iter("arg"):
                    if not arg.get("name"):
                        raise RuntimeError("Argument must have name")
                    response_args.append((arg.get("name"), arg.get("type")))
                response_msg = Message(f"{name}_response", response_args, "response")
                (events if child.tag == "request" else requests).append(response_msg)
                msg.response = response_msg
            (requests if child.tag == "request" else events).append(msg)
        elif child.tag == "type":
            passing = child.get("passing", "value")
            types[name] = passing
        elif child.tag == "include":
            includes.append(child.text)
        else:
            raise RuntimeError("Unrecognised Tag")

    if "ipc/connection.h" not in includes:
        includes.append("ipc/connection.h")
    # Now we want to number our messages.
    for i, req in enumerate(requests):
        req.number = i
    for i, evt in enumerate(events):
        evt.number = i
    return Interface(base_name, namespace, includes, requests, events, types)


def argument_list(interface: Interface, msg: Message, sending: bool):
    def argument(arg_name: str, arg_type: str):
        type_kind = interface.types.get(arg_type, "value")
        if type_kind == "value" or (not sending and type_kind == "move"):
            return f"{arg_type} {arg_name}"
        else:
            return f"const {arg_type}& {arg_name}"

    return ", ".join(argument(*a) for a in msg.arguments)


def ch(o: bool, y: str, n: str = ""):
    return y if o else n


def cho(o: bool, y, n):
    return y if o else n


def class_name(interface: Interface, is_server: bool, is_raw: bool, include_namespace: bool = True):
    return ch(interface.namespace and include_namespace, f"{interface.namespace}::") + interface.name + ch(is_server,
                                                                                                           "Server",
                                                                                                           "Client") + ch(
        is_raw, "Raw")


def generate_messages_enum(interface: Interface, is_server: bool):
    members = ",\n    ".join(m.name.upper() for m in cho(is_server, interface.events, interface.requests))
    return f"""enum class Messages: u32 {{
    {members},
    END_OF_MESSAGES
    }};"""


def generate_message_enum_value(interface: Interface, msg: Message, is_server: bool):
    return class_name(interface, is_server, True) + "::Messages::" + msg.name.upper()


def generate_enum_traits(interface: Interface, is_server: bool):
    return f"ipc::enum_traits<{class_name(interface, is_server, True)}::Messages, {class_name(interface, is_server, True)}::Messages::END_OF_MESSAGES>"


def render_class_interface(interface: Interface, is_server: bool, is_raw: bool):
    cls_name = class_name(interface, is_server, is_raw, False)

    # TODO: non-RAW
    def generate_message_interface(msg: Message, sending: bool):
        if sending:
            return f"void {msg.name}({argument_list(interface, msg, sending)});"
        else:
            return f"virtual void on_{msg.name}({argument_list(interface, msg, sending)}) = 0;"

    sending_members = '\n    '.join(
        generate_message_interface(msg, True) for msg in cho(is_server, interface.events, interface.requests))
    receiving_members = '\n    '.join(
        generate_message_interface(msg, False) for msg in cho(is_server, interface.requests, interface.events))

    return f"""class {cls_name}: public ipc::Connection {{
public:
    using Connection::Connection;
    {generate_messages_enum(interface, is_server)}
    {sending_members}
    {receiving_members}
protected:
    ErrorCode dispatch_message(u32 id, ipc::Message& buffer) override;
}};"""


def render_header(interface: Interface):
    includes = '\n'.join(f"#include <{inc}>" for inc in interface.includes)
    include_guard_name = f"BEKOS_IPC_GEN_{interface.name.upper()}"
    if interface.namespace:
        ns_begin = f"namespace {interface.namespace} {{"
        ns_end = "}"
    else:
        ns_begin = ""
        ns_end = ""

    raw_server = render_class_interface(interface, True, True)
    raw_client = render_class_interface(interface, False, True)

    return f"""
#ifndef {include_guard_name}
#define {include_guard_name}
{includes}
{ns_begin}

{raw_server}

{raw_client}

{ns_end}
#endif
"""


def generate_send_implementation(interface: Interface, is_server: bool, is_raw: bool):
    cls_name = class_name(interface, is_server, is_raw)

    def generate_imp(msg: Message):
        arguments = '\n    '.join(f"message.encode({n});" for n, _ in msg.arguments)
        return f"""
void {cls_name}::{msg.name}({argument_list(interface, msg, True)}) {{
    ipc::Message message{{{generate_enum_traits(interface, is_server)}::from_enum({generate_message_enum_value(interface, msg, is_server)})}};
    {arguments}
    send_message(message);
}}"""

    return '\n'.join(generate_imp(m) for m in cho(is_server, interface.events, interface.requests))


def generate_dispatch_implementation(interface: Interface, is_server: bool, is_raw: bool):
    fn_name = class_name(interface, is_server, is_raw) + "::dispatch_message"

    def pass_argument(arg: tuple[str, str]):
        match interface.types.get(arg[1], "value"):
            case "value" | "reference":
                return f"arg_{arg[0]}"
            case "move":
                return f"bek::move(arg_{arg[0]})"

    def generate_case(msg: Message):

        args = '\n'.join(f"auto arg_{n} = EXPECTED_TRY(buffer.decode<{t}>());" for n, t in msg.arguments)
        arg_list = ', '.join(pass_argument(a) for a in msg.arguments)
        return f"""case {generate_message_enum_value(interface, msg, not is_server)}: {{
        {args}
        on_{msg.name}({arg_list});
        return ESUCCESS;
        }}"""

    cases = '\n'.join(generate_case(m) for m in cho(is_server, interface.requests, interface.events))
    return f"""
ErrorCode {fn_name}(u32 id, ipc::Message& buffer) {{
    auto message_id = EXPECTED_TRY(({generate_enum_traits(interface, not is_server)}::to_enum(id)));
    switch (message_id) {{
        {cases}
        default: return EINVAL;
    }}
}}"""


def render_implementation(interface: Interface):
    return f"""
#include "{interface.name}.gen.h"

{generate_dispatch_implementation(interface, True, True)}
{generate_send_implementation(interface, True, True)}

{generate_dispatch_implementation(interface, False, True)}
{generate_send_implementation(interface, False, True)}
"""


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='ipcgen',
        description='Generates ipc stub files',
    )
    parser.add_argument('source')
    args = parser.parse_args()
    source_path = Path(args.source)
    base_name = source_path.stem

    tree = ET.parse(source_path)
    iface = parse_file(tree, base_name)

    with open(f'{base_name}.gen.h', 'w') as f:
        f.write(render_header(iface))

    with open(f'{base_name}.gen.cpp', 'w') as f:
        f.write(render_implementation(iface))
