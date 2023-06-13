import os
import subprocess
from pathlib import Path

def get_func_name(line):
    return line.split('(')[0].strip().split(' ')[-1].strip()

def get_macro_name(line):
    return ('one', [('yes', line.split(' ')[1].strip())])

def parse_expr(line):
    #print(line)
    line = line.strip()
    assert(line.startswith('#if '))
    line = line[4:]
    defs = line.split()
    res = []
    rel = 'one'
    #print(defs)
    for d in defs:
        if d.startswith('!'):
            res.append(('not', d[1+8: -1]))
        elif d.startswith('defined'):
            res.append(('yes', d[8: -1]))
        elif d == '&&':
            #assert(rel == '' or rel == 'and')
            rel = 'and'
        elif d == '||':
            #assert(rel == '' or rel == 'or')
            rel = 'or'
        else:
            assert False, f'unknown: {d}'  
    #assert(rel != '')
    #print((rel, res))
    return (rel, res)

def parse(path):
    res = {}
    macros = []
    prefix = ''
    with open(path) as f:
        for _line in f:
            if _line.strip().endswith('\\'):
                prefix += _line.strip()[:-1]
                continue
            line = prefix + _line
            prefix = ''
            if line.startswith(('GRIN_', 'void', 'bool', 'size_t', 'const')):
                func_name = get_func_name(line)
                res[func_name] = macros.copy()
            elif line.startswith('#ifdef'):
                assert(len(macros) == 0)
                macro_name = get_macro_name(line)
                macros.append(macro_name)
            elif line.startswith('#endif'):
                assert(len(macros) <= 1)
                if len(macros) == 1:
                    macros = macros[:-1]
            elif line.startswith('#if '):
                assert(len(macros) == 0)
                macro_name = parse_expr(line)
                macros.append(macro_name)
    return res

def to_rust(deps):
    if len(deps) == 0:
        return ''
    assert(len(deps) == 1)
    one_yes_foramt = '#[cfg(feature = \"{}\")]'
    one_not_foramt = '#[cfg(not(feature = \"{}\"))]'
    yes_format = 'feature = \"{}\"'
    not_format = 'not(feature = \"{}\")'
    all_format = '#[cfg(all({}))]'
    any_format = '#[cfg(any({}))]'

    deps = deps[0]
    if deps[0] == 'one':
        assert(len(deps[1]) == 1)
        if (deps[1][0][0] == 'yes'):
            return one_yes_foramt.format(deps[1][0][1].lower())
        else :
            return one_not_foramt.format(deps[1][0][1].lower())
    elif deps[0] == 'and':
        conds = [not_format.format(d[1].lower()) if d[0] == 'not' else yes_format.format(d[1].lower()) for d in deps[1]]
        return all_format.format(", ".join(conds))
    elif deps[0] == 'or':
        conds = [not_format.format(d[1].lower()) if d[0] == 'not' else yes_format.format(d[1].lower()) for d in deps[1]]
        return any_format.format(", ".join(conds))
    else:
        assert False, f'unknown: {deps}'
 
def snake_to_camel(s):
    if s.startswith(('GRIN_DATATYPE_', 'GRIN_DIRECTION_', 'GRIN_ERROR_CODE_')):
        return s.upper()
    return ''.join([w.capitalize() for w in s.split('_')])

def snake_to_camel_line(line):
    segs = line.split(' ')
    return ' '.join([snake_to_camel(s) if s.startswith('GRIN_') and s.find('NULL') == -1 else s for s in segs])

def static_replace(line):
    replaces = {
        '::std::os::raw::c_uint': 'u32',
        '::std::os::raw::c_int': 'i32',
        '::std::os::raw::c_ulonglong': 'u64',
        '::std::os::raw::c_longlong': 'i64',
    }
    for k in replaces:
        line = line.replace(k, replaces[k])
    return line


def rewrite(file, r, strip=49):
    with open(file) as f:
        lines = f.readlines()
    externc_flag = True
    need_ending_line = True
    meet_error_code = False
    with open(file, 'w') as f:
        for i, line in enumerate(lines):
            if i < strip:
                continue
            line = snake_to_camel_line(line)
            line = static_replace(line)
            if line.startswith('extern '):
                if externc_flag:
                    f.write('extern "C" {\n')
                    externc_flag = False
                continue
            if line.startswith('}'):
                if i < len(lines) - 1:
                    f.write('\n')
                else:
                    need_ending_line = False
                    f.write('}\n')
                continue
            if line.find('pub fn') != -1:
                func_name = line
                func_name = func_name[func_name.find('pub fn')+7:]
                func_name = func_name.split('(')[0]
                if func_name in r and r[func_name]:
                    f.write(f'    {r[func_name]}\n')
                f.write('    #[allow(unused)]\n')
                
            if line.find('RUST_KEEP') != -1:
                if need_ending_line:
                    f.write('}\n\n')
                segs = line.split('RUST_KEEP')
                for s in segs[1:]:
                    f.write(s[:s.find(';')+1])
                    f.write('\n\n')
                break
            if line.find('pub static mut grin_error_code: GrinErrorCode') != -1:
                continue
            f.write(line)
        

def parse_to_rs(path, dst):
    r = {}
    for f in path.glob('include/include/**/*.h'):
        print(f)
        r |= parse(f)
    #print(r)
    for k in r:
        r[k] = to_rust(r[k])
    #print(r)
    rewrite(f'{dst}.rs', r)

def parse_to_toml(path, dst):
    with open(path / 'predefine.h') as f:
        lines = f.readlines()
    states = ['none']
    enables = []
    deps = {}
    for line in lines:
        if line.startswith('// GRIN_DEFAULT_DISABLE'):
            assert(states[-1] == 'none')
            states.append('disable')
        elif line.startswith('// GRIN_STORAGE_ENABLE'):
            assert(states[-1] == 'none')
            states.append('enable')
        elif line.startswith('// GRIN_FEATURE_DEPENDENCY'):
            assert(states[-1] == 'none')
            states.append('dependency')
        elif line.startswith('// GRIN_END'):
            assert(len(states) > 1)
            states = states[:-1]
        else:
            status = states[-1]
            if status == 'enable':
                assert(line.startswith('#define '))
                enables.append(line[8:].strip().lower())
            elif status == 'disable':
                assert(line.startswith('#undef '))
                deps[line[7:].strip().lower()] = []
            elif status == 'dependency':
                if line.startswith('#ifdef'):
                    k = line.split(' ')[1].strip().lower()
                elif line.startswith('#define'):
                    v = line.split(' ')[1].strip().lower()
                    deps[k].append(v)
    #print(enables)
    #print(deps)
    with open('Cargo.toml', 'w') as f:
        f.write('[package]\n')
        f.write(f'name = \"{dst}\"\n')
        f.write('version = \"0.1.0\"\n')
        f.write('authors = [\"dijie\"]\n')
        f.write('\n')
        f.write('[features]\n')
        f.write('default = [{}]\n'.format(', '.join([f'\"{e}\"' for e in enables])))
        for k in deps:
            f.write(f'{k} = [')
            f.write(', '.join([f'\"{v}\"' for v in deps[k]]))
            f.write(']\n')

def bindgen(src, dst):
    subprocess.run(['bindgen', src, '-o', f'{dst}.rs'])

if __name__ == '__main__':
    src = 'gart_all.h'
    dst = 'grin_gart'
    path = Path('..')
    bindgen(src, dst)
    parse_to_rs(path, dst)
    parse_to_toml(path, dst)