# Python lldb formatters for parallel-hashmap
# tested witch clang 10 / lldb 9 & 10

# to install it, type the following command or put it in $HOME/.lldbinit:
# command script import "PATH_TO_SCRIPT/phmap_lldb.py"


import lldb
import os
import sys
import re

_MAX_CHILDREN = 250
_MAX_CTRL_INDEX = 1_000
_MODULE_NAME = os.path.basename(__file__).split(".")[0]


def _get_function_name(instance=None):
    res = f"{type(instance).__name__}." if instance else ""
    return res + sys._getframe(1).f_code.co_name


class flat_map_slot_type:
    CLASS_PATTERN = "^phmap::priv::raw_hash_set<phmap::priv::FlatHashMapPolicy.*>::slot_type$"
    HAS_SUMMARY = True
    IS_SYNTHETIC_PROVIDER = False

    @staticmethod
    def summary(valobj, _):
        try:
            valobj = valobj.GetChildMemberWithName('value')
            first = valobj.GetChildMemberWithName('first').GetSummary()
            if not first: first = "{...}"
            second = valobj.GetChildMemberWithName('second').GetSummary()
            if not second: second = "{...}"
            return f"{{{first}, {second}}}"
        except BaseException as ex:
            print(f"{_get_function_name()} -> {ex}")
        return ""


class node_map_slot_type:
    CLASS_PATTERN = r"^(" \
                    r"phmap::priv::raw_hash_set<phmap::priv::NodeHashMapPolicy.*>::slot_type" \
                    r"|" \
                    r"std::remove_cv<\s*std::pair\s*<.*>\s*>::type" \
                    r")$"
    HAS_SUMMARY = True
    IS_SYNTHETIC_PROVIDER = False

    @staticmethod
    def summary(valobj, _):
        try:
            first = valobj.GetChildMemberWithName('first').GetSummary()
            if not first: first = "{...}"
            second = valobj.GetChildMemberWithName('second').GetSummary()
            if not second: second = "{...}"
            return f"{{{first}, {second}}}"
        except BaseException as ex:
            print(f"{_get_function_name()} -> {ex}")
        return ""


class flat_hash_map_or_set:
    CLASS_PATTERN = "^phmap::flat_hash_(map|set)<.*>$"
    HAS_SUMMARY = True
    IS_SYNTHETIC_PROVIDER = True

    @staticmethod
    def summary(valobj, _):
        try:
            valobj = valobj.GetNonSyntheticValue()
            size = valobj.GetChildMemberWithName('size_').GetValueAsUnsigned()
            capacity = valobj.GetChildMemberWithName('capacity_').GetValueAsUnsigned()
            return f"size = {size} (capacity = {capacity})"
        except BaseException as ex:
            print(f"{_get_function_name()} -> {ex}")
        return ""

    def __init__(self, valobj, _):
        self.valobj = valobj
        self.slots_ = self.slot_type = self.ctrl_ = None
        self.size_ = self.capacity_ = self.slot_size = 0

    def num_children(self):
        return min(self.size_, _MAX_CHILDREN)

    def has_children(self):
        return True

    def update(self):
        try:
            self.size_ = self.valobj.GetChildMemberWithName('size_').GetValueAsUnsigned()
            self.capacity_ = self.valobj.GetChildMemberWithName('capacity_').GetValueAsUnsigned()
            self.slots_ = self.valobj.GetChildMemberWithName("slots_")
            self.slot_type = self.slots_.GetType().GetPointeeType()
            self.slot_size = self.slot_type.GetByteSize()
            self.ctrl_ = self.valobj.GetChildMemberWithName("ctrl_")
        except BaseException as ex:
            print(f"{_get_function_name(self)} -> {ex}")

    def get_child_index(self, name):
        try:
            if name in ('size_', 'capacity_'):
                return -1
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index):
        try:
            if index < 0:
                return None
            if index >= self.size_ or index >= _MAX_CHILDREN:
                return None
            real_idx = -1
            for idx in range(min(self.capacity_ + 3, _MAX_CTRL_INDEX)):
                ctrl = self.ctrl_.GetChildAtIndex(idx).GetValueAsSigned()
                if ctrl >= -1:
                    real_idx += 1
                    if real_idx == index:
                        return self.slots_.CreateChildAtOffset(f'[{index}]', real_idx * self.slot_size, self.slot_type)
        except BaseException as ex:
            print(f"{_get_function_name(self)} -> {ex}")
        return None


class parallel_flat_or_node_base:
    REGEX_EXTRACT_ARRAY_SIZE = re.compile(f"std::array\s*<.*,\s*(\d+)\s*>")

    @staticmethod
    def _get_size_and_capacity(valobj):
        try:
            sets = valobj.GetChildMemberWithName('sets_')
            # sets is an std::array<T, SIZE>.
            # It's not possible to get the size of the array with templates parameters
            # "set.GetType().GetTemplateArgumentType(1)" returns an "unsigned long" type but not the value
            # so we must extract it with a regex
            n_buckets = int(
                parallel_flat_or_node_base.REGEX_EXTRACT_ARRAY_SIZE.match(sets.GetType().GetName()).group(1))
            # this is dependent on the implementation of the standard library
            buckets = sets.GetChildMemberWithName('_M_elems')
            size = capacity = 0
            for idx in range(n_buckets):
                bucket = buckets.GetChildAtIndex(idx).GetChildMemberWithName('set_')
                size += bucket.GetChildMemberWithName('size_').GetValueAsUnsigned()
                capacity += bucket.GetChildMemberWithName('capacity_').GetValueAsUnsigned()
            return size, capacity, n_buckets
        except:
            return '?', '?', 0

    @staticmethod
    def summary(valobj, _):
        size, capacity, _ = parallel_flat_or_node_base._get_size_and_capacity(valobj)
        return f"size = {size} (capacity = {capacity})"

    def __init__(self, valobj, _):
        self.valobj = valobj
        self.buckets = self.slot_type = None
        self.size_ = self.capacity_ = self.n_buckets_ = self.slot_type = self.ctrl_size = 0

    def using_nodes(self):
        return False

    def num_children(self):
        return min(self.size_, _MAX_CHILDREN)

    def has_children(self):
        return True

    def update(self):
        try:
            self.size_, self.capacity_, self.n_buckets_ = self._get_size_and_capacity(self.valobj)
            self.buckets = self.valobj.GetChildMemberWithName('sets_').GetChildMemberWithName('_M_elems')
            bucket0 = self.buckets.GetChildAtIndex(0).GetChildMemberWithName('set_')
            self.slot_type = bucket0.GetChildMemberWithName('slots_').GetType().GetPointeeType()
            if  self.using_nodes():
                self.slot_type = self.slot_type.GetPointeeType()
            self.slot_size = self.slot_type.GetByteSize()
        except BaseException as ex:
            print(f"{_get_function_name(self)} -> {ex}")

    def get_child_index(self, name):
        try:
            if name in ('sets_'):
                return -1
            return int(name.lstrip('[').rstrip(']'))
        except:
            return -1

    def get_child_at_index(self, index):
        try:
            if index < 0:
                return None
            if index >= self.size_ or index >= _MAX_CHILDREN:
                return None
            real_idx = -1
            total_idx = 0
            for idx in range(self.n_buckets_):
                bucket = self.buckets.GetChildAtIndex(idx).GetChildMemberWithName('set_')
                capacity = bucket.GetChildMemberWithName("capacity_").GetValueAsUnsigned()
                if capacity:
                    slots_ = bucket.GetChildMemberWithName("slots_")
                    ctrl_ = bucket.GetChildMemberWithName("ctrl_")
                    for jdx in range(capacity):
                        ctrl = ctrl_.GetChildAtIndex(jdx).GetValueAsSigned()
                        if ctrl >= -1:
                            real_idx += 1
                            if real_idx == index:
                                if  self.using_nodes():
                                    return slots_.Dereference().CreateChildAtOffset(f'[{index}]', jdx * self.slot_size, self.slot_type)
                                return slots_.CreateChildAtOffset(f'[{index}]', jdx * self.slot_size, self.slot_type)
                    total_idx += capacity
                    if total_idx > _MAX_CHILDREN:
                        return None
        except BaseException as ex:
            print(f"{_get_function_name(self)} -> {ex}")
        return None


class parallel_flat_hash_map_or_set(parallel_flat_or_node_base):
    CLASS_PATTERN = "^phmap::parallel_flat_hash_(map|set)<.*>$"
    HAS_SUMMARY = True
    IS_SYNTHETIC_PROVIDER = True


class parallel_node_hash_map_or_set(parallel_flat_or_node_base):
    CLASS_PATTERN = "^phmap::parallel_node_hash_(map|set)<.*>$"
    HAS_SUMMARY = True
    IS_SYNTHETIC_PROVIDER = True

    def using_nodes(self):
        return True


def __lldb_init_module(debugger, internal_dict):
    for sp in (
            flat_map_slot_type,
            node_map_slot_type,
            flat_hash_map_or_set,
            parallel_flat_hash_map_or_set,
            parallel_node_hash_map_or_set,
    ):
        if sp.HAS_SUMMARY:
            debugger.HandleCommand(
                f'type summary add --regex "{sp.CLASS_PATTERN}" --python-function {_MODULE_NAME}.{sp.__name__}.summary '
                f'--category phmap --expand')
        if sp.IS_SYNTHETIC_PROVIDER:
            debugger.HandleCommand(
                f'type synthetic add --regex "{sp.CLASS_PATTERN}" --python-class {_MODULE_NAME}.{sp.__name__} '
                f'--category phmap')
    debugger.HandleCommand('type category enable phmap')
