"""Copy a named clip between GLBs with matching rigs, retaining the destination assets."""
import argparse
import copy
import json
from pathlib import Path
import struct


def read_glb(path):
    data = Path(path).read_bytes()
    magic, version, length = struct.unpack_from('<III', data)
    if magic != 0x46546C67 or version != 2 or length != len(data):
        raise ValueError('Invalid GLB header')
    size, kind = struct.unpack_from('<II', data, 12)
    if kind != 0x4E4F534A:
        raise ValueError('Missing JSON chunk')
    doc = json.loads(data[20:20 + size])
    offset = 20 + size
    size, kind = struct.unpack_from('<II', data, offset)
    if kind != 0x004E4942:
        raise ValueError('Missing binary chunk')
    return doc, data[offset + 8:offset + 8 + size]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source')
    parser.add_argument('destination')
    parser.add_argument('animation')
    parser.add_argument('--write', action='store_true')
    args = parser.parse_args()
    source, source_bin = read_glb(args.source)
    dest, dest_bin = read_glb(args.destination)
    clip = copy.deepcopy(next(a for a in source['animations'] if a['name'] == args.animation))
    names = {}
    joints = {joint for skin in dest['skins'] for joint in skin['joints']}
    for index, node in enumerate(dest['nodes']):
        if index not in joints:
            continue
        name = node.get('name')
        if name in names:
            raise ValueError(f'Duplicate destination node: {name}')
        names[name] = index
    source_parents = {child: node.get('name') for node in source['nodes'] for child in node.get('children', [])}
    dest_parents = {child: node.get('name') for node in dest['nodes'] for child in node.get('children', [])}
    for channel in clip['channels']:
        old_index = channel['target']['node']
        node = source['nodes'][old_index]
        index = names[node['name']]
        if source_parents.get(old_index) != dest_parents.get(index):
            raise ValueError(f'Rig hierarchy differs for {node["name"]}')
        channel['target']['node'] = index
    # Verify joint bind transforms too: matching names alone do not imply matching rigs.
    def binds(doc, binary):
        result = {}
        for skin in doc['skins']:
            accessor = doc['accessors'][skin['inverseBindMatrices']]
            view = doc['bufferViews'][accessor['bufferView']]
            for i, joint in enumerate(skin['joints']):
                offset = view.get('byteOffset', 0) + accessor.get('byteOffset', 0) + i * view.get('byteStride', 64)
                result[doc['nodes'][joint]['name']] = struct.unpack_from('<16f', binary, offset)
        return result
    source_binds, dest_binds = binds(source, source_bin), binds(dest, dest_bin)
    for name, matrix in dest_binds.items():
        if name not in source_binds or max(abs(a - b) for a, b in zip(matrix, source_binds[name])) > 0.001:
            raise ValueError(f'Bind pose differs for {name}')
    output = bytearray(dest_bin)
    accessors, views = {}, {}
    for sampler in clip['samplers']:
        for key in ('input', 'output'):
            old = sampler[key]
            if old not in accessors:
                accessor = copy.deepcopy(source['accessors'][old])
                if 'sparse' in accessor:
                    raise ValueError('Sparse animation accessors are not supported')
                old_view = accessor['bufferView']
                if old_view not in views:
                    view = copy.deepcopy(source['bufferViews'][old_view])
                    if view.get('buffer', 0) != 0:
                        raise ValueError('External animation buffer')
                    output.extend(b'\0' * (-len(output) % 4))
                    offset = view.get('byteOffset', 0)
                    view['byteOffset'] = len(output)
                    output.extend(source_bin[offset:offset + view['byteLength']])
                    views[old_view] = len(dest['bufferViews'])
                    dest['bufferViews'].append(view)
                accessor['bufferView'] = views[old_view]
                accessors[old] = len(dest['accessors'])
                dest['accessors'].append(accessor)
            sampler[key] = accessors[old]
    if any(a['name'] == args.animation for a in dest.get('animations', [])):
        raise ValueError('Destination already contains this animation')
    dest.setdefault('animations', []).append(clip)
    dest['buffers'][0]['byteLength'] = len(output)
    encoded = json.dumps(dest, separators=(',', ':')).encode()
    encoded += b' ' * (-len(encoded) % 4)
    output.extend(b'\0' * (-len(output) % 4))
    data = struct.pack('<III', 0x46546C67, 2, 28 + len(encoded) + len(output))
    data += struct.pack('<II', len(encoded), 0x4E4F534A) + encoded
    data += struct.pack('<II', len(output), 0x004E4942) + output
    print(f'{args.animation}: {len(clip["channels"])} channels; matching hierarchy and bind pose; {len(data)} bytes')
    if args.write:
        Path(args.destination).write_bytes(data)
        print('Animation added; existing meshes, materials, images and clips retained')


if __name__ == '__main__':
    main()
