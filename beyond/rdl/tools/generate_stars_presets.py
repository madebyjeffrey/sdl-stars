#!/usr/bin/env python3
"""Generate reusable setting traits and race objects from the source presets.

Typed trait templates describe independent choices. Presets apply them directly
inside object declarations; identical applications have the same identity. --check does not write files.
Requires the repository's existing PyYAML; no game sources are modified.
"""
import argparse
from pathlib import Path
import sys
import yaml

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parents[1] / 'examples/stars'
NAMES = ['Humanoid', 'Rabbitoid', 'Insectoid', 'Nucleotid', 'Silicanoid', 'Antetheral', 'Random']
PRTS = dict(zip(
    ['raCheapCol', 'raStealth', 'raAttack', 'raTerra', 'raDefend', 'raMines', 'raMassAccel', 'raStargate', 'raMacintosh', 'raNone'],
    ['hyper_expansion', 'super_stealth', 'war_monger', 'claim_adjuster', 'inner_strength', 'space_demolition', 'packet_physics', 'interstellar_traveler', 'alternate_reality', 'jack_of_all_trades']))
FLAGS = dict(zip(
    ['IFE', 'TT', 'ARM', 'ISB', 'GeneralizedResearch', 'UltimateRecycling', 'MineralAlchemy', 'NoRamscoops', 'CheapEngines', 'OBRM', 'NoAdvScanner', 'LowStartingPop', 'BleedingEdgeTech', 'RegeneratingShields'],
    ['ife', 'tt', 'arm', 'isb', 'gr', 'ur', 'ma', 'nrse', 'ce', 'obrm', 'nas', 'lsp', 'bet', 'rs']))
FIELDS = ['energy', 'weapons', 'propulsion', 'construction', 'electronics', 'biotechnology']
ECONOMY = [
    ('population_efficiency', 'PopulationResourceDivisor', 'rsResGen'),
    ('factory_output', 'FactoryOutput', 'rsFactProd'),
    ('factory_cost', 'FactoryCost', 'rsFactBuild'),
    ('factory_operation', 'FactoryOperationLimit', 'rsFactOperate'),
    ('mine_output', 'MineOutput', 'rsMineProd'),
    ('mine_cost', 'MineCost', 'rsMineBuild'),
    ('mine_operation', 'MineOperationLimit', 'rsMineOperate'),
]
HEADER = '(* Generated from data/races/predefined.yaml by tools/generate_stars_presets.py. *)\n'


def generate():
    rows = yaml.safe_load((ROOT / 'data/races/predefined.yaml').read_text())
    assert len(rows) == len(NAMES)
    files = {}
    settings = []

    def template(name, parameters, body):
        settings.append(f'trait {name}({parameters}) {{\n{body}\n}}\n')

    template('portrait', 'index: integer', '  provides Portrait = index')
    for axis in ['gravity', 'temperature', 'radiation']:
        template(axis, 'lower: integer, upper: integer',
            '  require lower >= 0 && upper <= 100 && lower < upper && upper - lower >= 2\n'
            f'  provides {axis.title()}Tolerance {{\n'
            '    range = { lower = lower; preferred = lower + (upper - lower) / 2; upper = upper; };\n  }')
        template(axis, 'mode: Immunity', f'  provides {axis.title()}Tolerance {{\n'
            '    range = { lower = -1; preferred = -1; upper = -1; };\n  }')
    template('growth', 'percent: integer', '  provides SelectedGrowth = percent\n  provides GrowthRate = real percent / 100.0')
    for prefix, component, key in ECONOMY:
        if key == 'rsResGen':
            template(prefix, 'colonists: integer',
                '  require colonists % 100 = 0\n  provides PopulationResourceDivisor = colonists / 100')
        else:
            template(prefix, 'value: integer', f'  provides {component} = value')
    for field in FIELDS:
        template(f'{field}_research', 'cost: ResearchCostClass',
            f'  provides {field.title()}ResearchCost = match cost with | expensive -> 0 | standard -> 1 | cheap -> 2')
    template('research_allocation', 'percent: integer', '  provides ResearchAllocationPercent = percent')
    template('leftover_points', 'choice: LeftoverChoice',
        '  provides LeftoverAllocation = match choice with | surface_minerals -> 0 | concentrations -> 1 | mines -> 2 | factories -> 3 | defenses -> 4 | option_5 -> 5 | option_6 -> 6')

    for index, (name, row) in enumerate(zip(NAMES, rows)):
        ident = name.lower()
        attr = row['rgAttr']
        choices = [f'portrait({row["iPlrBmp"]})']
        for i, axis in enumerate(['gravity', 'temperature', 'radiation']):
            values = []
            for array in ['rgEnvVarMin', 'rgEnvVar', 'rgEnvVarMax']:
                # Missing trailing YAML array elements are zero in the source.
                value = row[array][i] if i < len(row[array]) else 0
                values.append(-1 if value == 'envImmune' else value)
            lower, preferred, upper = values
            assert (lower == preferred == upper == -1) or preferred == lower + (upper - lower) // 2
            choices.append(f'{axis}(immune)' if lower == -1 else f'{axis}({lower}, {upper})')
        growth = row['pctIdealGrowth']
        choices.append(f'growth({growth})')
        for prefix, component, key in ECONOMY:
            value = attr[key]
            choices.append(f'{prefix}({value * 100 if key == "rsResGen" else value})')
        for i, field in enumerate(FIELDS):
            value = attr[f'rsTechBonus{i + 1}']
            label = ['expensive', 'standard', 'cheap'][value]
            choices.append(f'{field}_research({label})')
        choices.append(f'research_allocation({row["pctResearch"]})')
        leftover = attr['rsUseLeftover']
        label = ['surface_minerals', 'concentrations', 'mines', 'factories', 'defenses'][leftover] if leftover < 5 else f'option_{leftover}'
        choices.append(f'leftover_points({label})')
        choices.append('standard_capacity')
        extra = []
        lesser = []
        if ident != 'random':
            for flag in row.get('grbitAttr', []):
                suffix = flag.removeprefix('grbitRace')
                if suffix == 'Tech3':
                    extra.append('higher_starting_technology')
                elif suffix == 'CheapFact':
                    extra.append('cheaper_factories')
                else:
                    lesser.append(f'lrt_{FLAGS[suffix]}')
        ty = 'RandomRaceTemplate' if ident == 'random' else 'Race'
        text = HEADER + 'import stars.settings\n\n'
        text += f'{ty} {ident} = {{\n'
        text += f'  name = "{name}";\n  plural_name = "{name}s";\n  source_index = {index};\n'
        if ident != 'random':
            choices.append(PRTS[attr['rsMajorAdv']])
        text += ''.join(f'  {choice}\n' for choice in choices + extra + lesser)
        text += '}\n'
        files[f'predefined/{ident}.rdl'] = text
    files['settings.rdl'] = (HEADER + 'import design\n\n'
        '(* Declarative templates: typed inputs supply component values.\n'
        '   Habitat uses the source 0..100 coordinates; population efficiency\n'
        '   counts colonists/resource, not hundreds. *)\n\n'
        'type Immunity = | immune\n'
        'type ResearchCostClass = | expensive | standard | cheap\n'
        'type LeftoverChoice = | surface_minerals | concentrations | mines | factories | defenses | option_5 | option_6\n\n'
        + '\n'.join(settings))
    files['predefined.rdl'] = ('(* Six fixed designs plus the Random generation template. *)\n'
        + ''.join(f'import predefined.{name.lower()}\n' for name in NAMES))
    return files


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    stale = []
    for name, content in generate().items():
        target = OUT / name
        if args.check:
            if not target.exists() or target.read_text() != content:
                stale.append(str(target))
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(content)
    if stale:
        sys.exit('Stale generated RDL facts:\n' + '\n'.join(stale))


if __name__ == '__main__':
    main()
