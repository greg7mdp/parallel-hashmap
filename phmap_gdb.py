# Python GDB formatters for parallel-hashmap
# tested with GCC 10.2 / GDB 9.2
# to install it, ensure the script location is in the Python path
# and type the following command (or put it in $HOME/.gdbinit):

# python
# import phmap_gdb
# end


import gdb.printing


def counter():
    i = 0
    while(True):
        yield str(i)
        i += 1


def slot_iterator(base_obj):
    index = -1
    n_items = 0
    size = int(base_obj["size_"])
    while n_items < size:
        index += 1
        if int(base_obj["ctrl_"][index]) < 0:
            continue

        n_items += 1
        yield base_obj["slots_"][index]


def parallel_slot_iterator(base_obj):
    array = base_obj["sets_"]
    array_len = int(array.type.template_argument(1))
    for index in range(array_len):
        obj = array["_M_elems"][index]["set_"]
        yield from slot_iterator(obj)


def flat_map_iterator(name, item):
    yield (next(name), item["value"]["first"])
    yield (next(name), item["value"]["second"])


def flat_set_iterator(name, item):
    yield (next(name), item)


def node_map_iterator(name, item):
    yield (next(name), item.dereference()["first"])
    yield (next(name), item.dereference()["second"])


def node_set_iterator(name, item):
    yield (next(name), item.dereference())


def traverse(iterator, slot_type_iterator):
    name = counter()
    for item in iterator:
        yield from slot_type_iterator(name, item)


def parallel_size(parallel_hash_obj):
    array = parallel_hash_obj["sets_"]
    array_len = int(array.type.template_argument(1))
    size = 0
    for index in range(array_len):
        size += array["_M_elems"][index]["set_"]["size_"]

    return size


class FlatMapPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(slot_iterator(self.val), flat_map_iterator)

    def to_string(self):
        return f"phmap::flat_hash_map with {int(self.val['size_'])} elements"

    def display_hint(self):
        return "map"


class FlatSetPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(slot_iterator(self.val), flat_set_iterator)

    def to_string(self):
        return f"phmap::flat_hash_set with {int(self.val['size_'])} elements"

    def display_hint(self):
        return "array"


class NodeMapPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(slot_iterator(self.val), node_map_iterator)

    def to_string(self):
        return f"phmap::node_hash_map with {int(self.val['size_'])} elements"

    def display_hint(self):
        return "map"


class NodeSetPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(slot_iterator(self.val), node_set_iterator)

    def to_string(self):
        return f"phmap::node_hash_set with {int(self.val['size_'])} elements"

    def display_hint(self):
        return "array"


class ParallelFlatMapPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(parallel_slot_iterator(self.val), flat_map_iterator)

    def to_string(self):
        return f"phmap::parallel_flat_hash_map with {parallel_size(self.val)} elements"

    def display_hint(self):
        return "map"


class ParallelFlatSetPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(parallel_slot_iterator(self.val), flat_set_iterator)

    def to_string(self):
        return f"phmap::parallel_flat_hash_set with {parallel_size(self.val)} elements"

    def display_hint(self):
        return "array"


class ParallelNodeMapPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(parallel_slot_iterator(self.val), node_map_iterator)

    def to_string(self):
        return f"phmap::parallel_node_hash_map with {parallel_size(self.val)} elements"

    def display_hint(self):
        return "map"


class ParallelNodeSetPrinter:
    def __init__(self, val):
        self.val = val

    def children(self):
        return traverse(parallel_slot_iterator(self.val), node_set_iterator)

    def to_string(self):
        return f"phmap::parallel_node_hash_set with {parallel_size(self.val)} elements"

    def display_hint(self):
        return "array"


def build_pretty_printer():
    pp = gdb.printing.RegexpCollectionPrettyPrinter("phmap")
    pp.add_printer('flat_hash_map', '^phmap::flat_hash_map<.*>$', FlatMapPrinter)
    pp.add_printer('flat_hash_set', '^phmap::flat_hash_set<.*>$', FlatSetPrinter)
    pp.add_printer('node_hash_map', '^phmap::node_hash_map<.*>$', NodeMapPrinter)
    pp.add_printer('node_hash_set', '^phmap::node_hash_set<.*>$', NodeSetPrinter)
    pp.add_printer('parallel_flat_hash_map', '^phmap::parallel_flat_hash_map<.*>$', ParallelFlatMapPrinter)
    pp.add_printer('parallel_flat_hash_set', '^phmap::parallel_flat_hash_set<.*>$', ParallelFlatSetPrinter)
    pp.add_printer('parallel_node_hash_map', '^phmap::parallel_node_hash_map<.*>$', ParallelNodeMapPrinter)
    pp.add_printer('parallel_node_hash_set', '^phmap::parallel_node_hash_set<.*>$', ParallelNodeSetPrinter)
    return pp


gdb.printing.register_pretty_printer(gdb.current_objfile(), build_pretty_printer())
