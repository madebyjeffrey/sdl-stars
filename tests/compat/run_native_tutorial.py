"""Run bounded native tutorial scenarios in isolated game/settings storage.

Passing this scenario is not evidence that the complete tutorial passes.
Artifacts survive outside the temporary game directory, including on failure.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile


def main():
    executable = Path(sys.argv[1]).resolve()
    artifacts = Path(sys.argv[2]).resolve()
    scenario = sys.argv[3] if len(sys.argv) > 3 else "smoke"
    assert scenario in ("smoke", "first-year", "two-years", "scouting", "mining", "colonization", "logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"), "unknown bounded tutorial scenario"
    page = {"smoke": 3, "first-year": 6, "two-years": 7, "scouting": 10, "mining": 11, "colonization": 13, "logistics": 17, "colonizer-production": 19, "second-colonization": 21, "transport-cycle": 23, "expansion": 26, "fleet-split": 27, "research-cycle": 30, "oxygen-colonization": 32, "production-templates": 34, "battle-cycle": 38, "terraform-cycle": 41, "design-cycle": 45, "automation-cycle": 46, "report-cycle": 48, "merge-cycle": 50}[scenario]
    timeout = {"smoke": 35, "first-year": 50, "two-years": 80, "scouting": 120, "mining": 120, "colonization": 180, "logistics": 240, "colonizer-production": 270, "second-colonization": 300, "transport-cycle": 360, "expansion": 420, "fleet-split": 450, "research-cycle": 510, "oxygen-colonization": 570, "production-templates": 630, "battle-cycle": 750, "terraform-cycle": 810, "design-cycle": 930, "automation-cycle": 990, "report-cycle": 1110, "merge-cycle": 1230}[scenario]
    turn = 18 if scenario == "merge-cycle" else 16 if scenario == "report-cycle" else 15 if scenario == "automation-cycle" else 14 if scenario == "design-cycle" else 13 if scenario == "terraform-cycle" else 12 if scenario == "battle-cycle" else 11 if scenario == "production-templates" else 10 if scenario == "oxygen-colonization" else 9 if scenario == "research-cycle" else 8 if scenario in ("expansion", "fleet-split") else 7 if scenario == "transport-cycle" else 6 if scenario == "second-colonization" else 5 if scenario == "colonizer-production" else 4 if scenario == "logistics" else 3 if scenario == "colonization" else 2 if scenario in ("two-years", "scouting", "mining") else 1 if scenario != "smoke" else 0
    generations = turn
    artifacts.mkdir(parents=True, exist_ok=True)
    for name in ("events.jsonl", "result.json", "client.log", "launcher-failure.txt", "year-2400-orders.json", "year-2401-orders.json", "year-2402-scout-orders.json", "year-2402-mining-orders.json", "year-2402-orders.json", "year-2403-start-orders.json", "year-2403-orders.json", "year-2404-start-orders.json", "year-2404-orders.json", "year-2405-start-orders.json", "year-2405-cargo.json", "year-2405-orders.json", "year-2406-start-orders.json", "year-2406-copied-orders.json", "year-2406-orders.json", "year-2407-start-orders.json", "year-2407-cargo.json", "year-2407-orders.json", "enemy-design.json", "year-2408-start-orders.json", "year-2408-after-transfer.json", "year-2408-split-cargo.json", "year-2408-before-split-all.json", "year-2408-split-orders.json", "year-2408-drag-orders.json", "year-2408-scout-orders.json", "year-2408-orders.json", "wallaby-radiation.json", "radiation-benefit.json", "year-2409-start-orders.json", "year-2409-research.json", "year-2409-cargo.json", "year-2409-orders.json", "year-2410-start-orders.json", "year-2410-shaggy-queue.json", "year-2410-imported-template.json", "year-2410-template-orders.json", "year-2410-orders.json", "year-2411-start-orders.json", "year-2411-colonizer-cargo.json", "year-2411-troop-cargo.json", "year-2411-mines.json", "year-2411-research.json", "year-2411-orders.json", "battle-open.json", "battle-playing.json", "battle-end.json", "battle-rewound.json", "battle-stepped.json", "salvage.json", "year-2412-start-orders.json", "year-2412-terraform-orders.json", "year-2412-teamster-orders.json", "year-2412-orders.json", "year-2413-start-orders.json", "year-2413-cargo.json", "year-2413-transport.json", "year-2413-research.json", "year-2413-miner-queue.json", "year-2413-mines.json", "year-2413-orders.json", "year-2414-start-orders.json", "year-2414-terraform-queue.json", "year-2414-imported-template.json", "year-2414-orders.json", "year-2415-start-orders.json", "year-2415-cargo.json", "year-2415-transport.json", "year-2415-research.json", "year-2415-orders.json", "value-report.json", "year-2416-start-orders.json", "year-2416-merge.json", "year-2416-production.json", "year-2416-research.json", "year-2416-orders.json", "year-2417-start-orders.json", "year-2417-orders.json"):
        (artifacts / name).unlink(missing_ok=True)
    for name in ("start", "rejected", "page-two", "page-three", "before-generate", "year-2401", "year-2402", "before-generate-2401", "scouting", "mining", "summary-popup", "before-generate-2402", "year-2403", "before-generate-2403", "year-2404", "before-generate-2404", "year-2405", "habitability", "before-generate-2405", "year-2406", "before-generate-2406", "year-2407", "red-storm-view", "enemy-summary", "enemy-design-popup", "before-generate-2407", "year-2408", "ship-transfer", "fleet-split", "waypoint-drag", "wallaby-radiation", "research-benefits", "radiation-benefit", "before-generate-2408", "year-2409", "oxygen-summary", "mozart-waypoint", "before-generate-2409", "year-2410", "shaggy-dog-summary", "template-open", "template-imported", "template-owner-restored", "template-closed", "bloop-summary", "before-generate-2410", "year-2411", "technology-browser", "technology-browser-closed", "battle-open", "battle-playing", "battle-closed", "before-generate-2411", "year-2412", "armed-probe-message", "armed-probe-goto", "missing-menu", "salvage-summary", "find-teamster-open", "find-teamster-entered", "find-teamster-closed", "before-generate-2412", "year-2413", "mini-miner-designed", "mini-miner-part", "production-items", "before-generate-2413", "year-2414", "terraform-template-open", "terraform-template-imported", "terraform-template-owner-restored", "terraform-template-closed", "before-generate-2414", "year-2415", "value-report-open", "value-report-sorted", "value-report-closed", "propulsion-research-open", "before-generate-2415", "year-2416", "before-generate-2416", "year-2417", "before-generate-2417", "year-2418", "failure"):
        for kind in ("hierarchy", "map"):
            (artifacts / f"{name}-{kind}.json").unlink(missing_ok=True)
    with tempfile.TemporaryDirectory(prefix="stars-native-tutorial-") as directory:
        work = Path(directory)
        env = dict(os.environ, STARS_SETTINGS_DIR=str(work))
        try:
            with (artifacts / "client.log").open("w", encoding="utf-8") as log:
                result = subprocess.run([str(executable), f"--{scenario}"], cwd=work, env=env,
                                        stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
        except (subprocess.TimeoutExpired, OSError) as failure:
            (artifacts / "launcher-failure.txt").write_text(str(failure), encoding="utf-8")
            raise
        finally:
            for path in work.iterdir():
                if path.suffix in (".json", ".jsonl"):
                    shutil.copyfile(path, artifacts / path.name)
        print((artifacts / "client.log").read_text(encoding="utf-8"))
        assert result.returncode == (2 if scenario != "smoke" else 0), f"native scenario failed; artifacts: {artifacts}"
        outcome = json.loads((work / "result.json").read_text())
        assert outcome["status"] == ("partial" if scenario != "smoke" else "passed") and outcome["scenario"] == scenario
        assert outcome["fullTutorial"] is False and outcome["rejectedGenerate"] is True
        events = [json.loads(line) for line in (work / "events.jsonl").read_text().splitlines()]
        assert events[-1]["event"] == (f"{scenario}-partial" if scenario != "smoke" else "smoke-passed")
        assert events[-1]["page"] == page
        assert all(event["autoComplete"] == 0 and event["turn"] <= turn for event in events)
        assert any(event["event"] == "read-message" for event in events)
        for name in ("start", "rejected", "page-two", "page-three"):
            assert json.loads((work / f"{name}-hierarchy.json").read_text())["windows"]
            assert json.loads((work / f"{name}-map.json").read_text())["objects"]
        if scenario != "smoke":
            assert outcome["pages"] == page and events[-1]["turn"] == turn
            assert {event["page"] for event in events if event["active"]} == set(range(1, page + 1))
            ready = [event for event in events if event["event"] == "generate-ready"]
            assert len(ready) == generations and ready[0]["turn"] == 0 and ready[0]["turnDone"] == 1
            checkpoint = json.loads((work / "year-2400-orders.json").read_text())
            assert checkpoint["schema"] == 1 and checkpoint["turn"] == 0 and checkpoint["turnDone"] == 1
            assert checkpoint["research"] == [1, 6, 15]
            fleets = {fleet["id"]: fleet["orders"] for fleet in checkpoint["fleets"]}
            planets = {obj["id"]: obj["universe"] for obj in
                       json.loads((work / "before-generate-map.json").read_text())["objects"] if obj["kind"] == 1}
            for fleet_id, planet_id in ((0, 12), (1, 16), (4, 15)):
                assert len(fleets[fleet_id]) == 2
                assert fleets[fleet_id][1]["kind"] == 1 and fleets[fleet_id][1]["id"] == planet_id
                assert fleets[fleet_id][1]["task"] == 0
                assert fleets[fleet_id][1]["position"] == planets[planet_id]
            assert json.loads((work / "year-2401-map.json").read_text())["turn"] == 1
            if scenario in ("two-years", "scouting", "mining", "colonization", "logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2401-orders.json").read_text())
                assert checkpoint["turn"] == 1 and checkpoint["turnDone"] == 1
                assert checkpoint["production"] == [{"planet": 13, "kind": 1, "item": 7, "quantity": 20}]
                assert ready[1]["turn"] == 1 and ready[1]["turnDone"] == 1
                assert json.loads((work / "year-2402-map.json").read_text())["turn"] == 2
            if scenario in ("scouting", "mining", "colonization", "logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2402-scout-orders.json").read_text())
                assert checkpoint["turn"] == 2 and checkpoint["turnDone"] == 0
                fleets = {fleet["id"]: fleet["orders"] for fleet in checkpoint["fleets"]}
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "scouting-map.json").read_text())["objects"] if obj["kind"] == 1}
                for fleet_id, route in ((0, [9, 3, 8, 5, 2]), (1, [21, 19, 20, 7]), (4, [14, 17, 18, 23, 22])):
                    assert len(fleets[fleet_id]) == len(route) + 1
                    for order, planet_id in zip(fleets[fleet_id][1:], route):
                        assert (order["kind"], order["id"], order["task"], order["position"]) == (1, planet_id, 0, planets[planet_id])
            if scenario in ("mining", "colonization", "logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2402-mining-orders.json").read_text())
                assert checkpoint["turn"] == 2 and checkpoint["turnDone"] == 0
                orders = next(fleet["orders"] for fleet in checkpoint["fleets"] if fleet["id"] == 5)
                assert len(orders) == 2 and (orders[1]["kind"], orders[1]["id"], orders[1]["task"]) == (1, 12, 3)
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "mining-map.json").read_text())["objects"] if obj["kind"] == 1}
                assert orders[1]["position"] == planets[12]
            if scenario in ("colonization", "logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                assert len([event for event in events if event["event"] == "summary-popup"]) == 1
                popup = json.loads((work / "summary-popup-hierarchy.json").read_text())["windows"]
                assert any(window["class"] == "starspopup" and window["visible"] and window["captured"] for window in popup)
                before = json.loads((work / "before-generate-2402-hierarchy.json").read_text())["windows"]
                assert not any(window["class"] == "starspopup" and window["visible"] for window in before)
                checkpoint = json.loads((work / "year-2402-orders.json").read_text())
                assert checkpoint["turn"] == 2 and checkpoint["turnDone"] == 1
                assert ready[2]["turn"] == 2 and ready[2]["turnDone"] == 1
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                assert fleets[2]["cargo"][:4] == [0, 0, 0, 25]
                orders = fleets[2]["orders"]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2402-map.json").read_text())["objects"] if obj["kind"] == 1}
                assert len(orders) == 2 and (orders[1]["kind"], orders[1]["id"], orders[1]["task"], orders[1]["position"]) == (1, 16, 2, planets[16])
                previous = json.loads((work / "year-2402-mining-orders.json").read_text())
                for fleet_id in (0, 1, 4, 5):
                    assert fleets[fleet_id]["orders"] == next(fleet["orders"] for fleet in previous["fleets"] if fleet["id"] == fleet_id)
                assert json.loads((work / "year-2403-map.json").read_text())["turn"] == 3
            if scenario in ("logistics", "colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2403-orders.json").read_text())
                assert checkpoint["turn"] == 3 and checkpoint["turnDone"] == 1
                assert checkpoint["filteredFactories"] is True
                assert json.loads((work / "year-2403-start-orders.json").read_text())["filteredFactories"] is False
                assert ready[3]["turn"] == 3 and ready[3]["turnDone"] == 1
                assert checkpoint["production"] == [
                    {"planet": 13, "kind": 1, "item": 1, "quantity": 30},
                    {"planet": 16, "kind": 1, "item": 7, "quantity": 3},
                    {"planet": 16, "kind": 1, "item": 8, "quantity": 3}]
                assert {item["planet"]: item["leftoverResearch"] for item in checkpoint["planetFlags"]} == {13: 0, 16: 1}
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                assert fleets[3]["cargo"][:4] == [0, 0, 0, 210]
                orders = fleets[3]["orders"]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2403-map.json").read_text())["objects"] if obj["kind"] == 1}
                assert len(orders) == 2 and (orders[1]["kind"], orders[1]["id"], orders[1]["task"], orders[1]["position"]) == (1, 16, 1, planets[16])
                assert orders[1]["transport"] == [{"action": action, "quantity": 0} for action in [2, 2, 2, 2, 7]]
                previous = {fleet["id"]: fleet for fleet in json.loads((work / "year-2403-start-orders.json").read_text())["fleets"]}
                assert len(previous[0]["orders"]) == 6 and len(fleets[0]["orders"]) == 5
                assert fleets[0]["orders"] == previous[0]["orders"][:1] + previous[0]["orders"][2:]
                for fleet_id in previous.keys() - {0, 3}:
                    assert fleets[fleet_id]["orders"] == previous[fleet_id]["orders"]
                assert json.loads((work / "year-2404-map.json").read_text())["turn"] == 4
            if scenario in ("colonizer-production", "second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2404-orders.json").read_text())
                previous = json.loads((work / "year-2404-start-orders.json").read_text())
                assert checkpoint["turn"] == 4 and checkpoint["turnDone"] == 1
                assert ready[4]["turn"] == 4 and ready[4]["turnDone"] == 1
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                previous_fleets = {fleet["id"]: fleet for fleet in previous["fleets"]}
                assert len(previous_fleets[4]["orders"]) == 6 and len(fleets[4]["orders"]) == 5
                assert previous_fleets[4]["orders"][1]["id"] == 14
                assert fleets[4]["orders"] == previous_fleets[4]["orders"][:1] + previous_fleets[4]["orders"][2:]
                for fleet_id in previous_fleets.keys() - {4}:
                    assert fleets[fleet_id]["orders"] == previous_fleets[fleet_id]["orders"]
                queue = [item for item in checkpoint["production"] if item["planet"] == 13]
                previous_queue = [item for item in previous["production"] if item["planet"] == 13]
                assert len(previous_queue) == 2 and len(queue) == 3
                assert queue == previous_queue[:1] + [{"planet": 13, "kind": 2, "item": 2, "quantity": 1}] + previous_queue[1:]
                assert [item for item in checkpoint["production"] if item["planet"] != 13] == [item for item in previous["production"] if item["planet"] != 13]
                assert checkpoint["planetFlags"] == previous["planetFlags"]
                assert checkpoint["research"] == previous["research"]
                assert checkpoint["filteredFactories"] == previous["filteredFactories"]
                assert json.loads((work / "year-2405-map.json").read_text())["turn"] == 5
            if scenario in ("second-colonization", "transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2405-orders.json").read_text())
                previous = json.loads((work / "year-2405-start-orders.json").read_text())
                cargo = json.loads((work / "year-2405-cargo.json").read_text())
                assert checkpoint["turn"] == 5 and checkpoint["turnDone"] == 1
                assert ready[5]["turn"] == 5 and ready[5]["turnDone"] == 1
                assert checkpoint["scannerMode"] == 0 and checkpoint["nextMessage"] == -1
                modes = [event for event in events if event["event"] in ("scanner-habitability", "scanner-normal")]
                assert [event["scannerMode"] for event in modes] == [3, 0]
                assert all(event["turn"] == 5 for event in modes)
                assert json.loads((work / "habitability-map.json").read_text())["objects"]
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                previous_fleets = {fleet["id"]: fleet for fleet in previous["fleets"]}
                loaded_fleets = {fleet["id"]: fleet for fleet in cargo["fleets"]}
                assert previous_fleets[2]["cargo"][:4] == [0, 0, 0, 0]
                assert loaded_fleets[2]["cargo"][:4] == [0, 0, 0, 25]
                assert fleets[2]["cargo"][:4] == [0, 0, 0, 25]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2405-map.json").read_text())["objects"] if obj["kind"] == 1}
                orders = fleets[2]["orders"]
                assert len(previous_fleets[2]["orders"]) == 1 and len(orders) == 2
                assert (orders[1]["kind"], orders[1]["id"], orders[1]["task"], orders[1]["position"]) == (1, 14, 2, planets[14])
                orders = fleets[3]["orders"]
                assert len(previous_fleets[3]["orders"]) == 1 and len(orders) == 2
                assert (orders[1]["kind"], orders[1]["id"], orders[1]["position"]) == (1, 13, planets[13])
                assert len(previous_fleets[0]["orders"]) == 5 and len(fleets[0]["orders"]) == 4
                assert previous_fleets[0]["orders"][1]["id"] == 3
                assert fleets[0]["orders"] == previous_fleets[0]["orders"][:1] + previous_fleets[0]["orders"][2:]
                for fleet_id in previous_fleets.keys() - {0, 2, 3}:
                    assert fleets[fleet_id]["orders"] == previous_fleets[fleet_id]["orders"]
                for fleet_id in previous_fleets.keys() - {2}:
                    assert fleets[fleet_id]["cargo"] == previous_fleets[fleet_id]["cargo"]
                for field in ("production", "planetFlags", "research", "filteredFactories"):
                    assert checkpoint[field] == previous[field]
                assert json.loads((work / "year-2406-map.json").read_text())["turn"] == 6
            if scenario in ("transport-cycle", "expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2406-orders.json").read_text())
                previous = json.loads((work / "year-2406-start-orders.json").read_text())
                copied = json.loads((work / "year-2406-copied-orders.json").read_text())
                assert checkpoint["turn"] == 6 and checkpoint["turnDone"] == 1
                assert ready[6]["turn"] == 6 and ready[6]["turnDone"] == 1
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                previous_fleets = {fleet["id"]: fleet for fleet in previous["fleets"]}
                copied_fleets = {fleet["id"]: fleet for fleet in copied["fleets"]}
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2406-map.json").read_text())["objects"] if obj["kind"] == 1}
                assert previous_fleets[3]["repeatOrders"] is False and fleets[3]["repeatOrders"] is True
                assert len(previous_fleets[3]["orders"]) == 1 and len(fleets[3]["orders"]) == 3
                assert fleets[3]["orders"][0] == previous_fleets[3]["orders"][0]
                for index, planet_id, actions in ((1, 12, [1, 1, 1, 0, 7]), (2, 13, [2, 2, 2, 2, 7])):
                    order = fleets[3]["orders"][index]
                    assert (order["kind"], order["id"], order["task"], order["position"]) == (1, planet_id, 1, planets[planet_id])
                    assert order["transport"] == [{"action": action, "quantity": 0} for action in actions]
                load = [{"action": action, "quantity": 0} for action in [1, 1, 1, 0, 7]]
                assert copied_fleets[3]["orders"][1]["transport"] == load
                assert copied_fleets[3]["orders"][2]["transport"] == load
                assert copied_fleets[3]["repeatOrders"] is False
                for fleet_id in previous_fleets.keys() - {3}:
                    assert fleets[fleet_id] == previous_fleets[fleet_id]
                assert fleets[3]["cargo"] == previous_fleets[3]["cargo"]
                queue = [item for item in checkpoint["production"] if item["planet"] == 13]
                previous_queue = [item for item in previous["production"] if item["planet"] == 13]
                assert len(previous_queue) == 2 and queue == previous_queue[:1] + [{"planet": 13, "kind": 2, "item": 2, "quantity": 1}] + previous_queue[1:]
                assert [item for item in checkpoint["production"] if item["planet"] != 13] == [item for item in previous["production"] if item["planet"] != 13]
                for field in ("planetFlags", "research", "filteredFactories", "scannerMode"):
                    assert checkpoint[field] == previous[field]
                assert json.loads((work / "year-2407-map.json").read_text())["turn"] == 7
            if scenario in ("expansion", "fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoint = json.loads((work / "year-2407-orders.json").read_text())
                previous = json.loads((work / "year-2407-start-orders.json").read_text())
                cargo = json.loads((work / "year-2407-cargo.json").read_text())
                assert checkpoint["turn"] == 7 and checkpoint["turnDone"] == 1
                assert ready[7]["turn"] == 7 and ready[7]["turnDone"] == 1
                assert checkpoint["scannerMode"] == 0 and checkpoint["nextMessage"] == -1
                modes = [event for event in events if event["event"] in ("expansion-habitability", "expansion-normal")]
                assert [event["scannerMode"] for event in modes] == [3, 0]
                assert all(event["turn"] == 7 for event in modes)
                assert previous["filteredMines"] is False and checkpoint["filteredMines"] is True
                assert checkpoint["filteredFactories"] == previous["filteredFactories"]
                assert checkpoint["research"] == previous["research"]
                fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                previous_fleets = {fleet["id"]: fleet for fleet in previous["fleets"]}
                loaded_fleets = {fleet["id"]: fleet for fleet in cargo["fleets"]}
                assert previous_fleets[6]["cargo"][:4] == [0, 0, 0, 0]
                assert loaded_fleets[6]["cargo"][:4] == fleets[6]["cargo"][:4] == [0, 0, 0, 25]
                orders = fleets[6]["orders"]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2407-map.json").read_text())["objects"] if obj["kind"] == 1}
                assert len(previous_fleets[6]["orders"]) == 1 and len(orders) == 2
                assert (orders[1]["kind"], orders[1]["id"], orders[1]["task"], orders[1]["position"]) == (1, 18, 2, planets[18])
                for fleet_id in previous_fleets.keys() - {6}:
                    assert fleets[fleet_id] == previous_fleets[fleet_id]
                assert checkpoint["production"] == [
                    {"planet": 13, "kind": 1, "item": 7, "quantity": 1},
                    {"planet": 13, "kind": 2, "item": 2, "quantity": 3},
                    {"planet": 13, "kind": 2, "item": 0, "quantity": 2},
                    {"planet": 13, "kind": 1, "item": 1, "quantity": 30},
                    {"planet": 16, "kind": 1, "item": 1, "quantity": 10},
                    {"planet": 16, "kind": 1, "item": 0, "quantity": 10}]
                assert {item["planet"]: item["leftoverResearch"] for item in checkpoint["planetFlags"]} == {13: 0, 16: 1}
                summary = json.loads((work / "enemy-summary-map.json").read_text())["objects"]
                assert any(obj["kind"] == 2 and obj["id"] == 512 and obj["scanSelected"] for obj in summary)
                assert len([event for event in events if event["event"] == "enemy-design-popup"]) == 1
                popup = json.loads((work / "enemy-design-popup-hierarchy.json").read_text())["windows"]
                assert any(window["class"] == "starspopup" and window["visible"] and window["captured"] for window in popup)
                before = json.loads((work / "before-generate-2407-hierarchy.json").read_text())["windows"]
                assert not any(window["class"] == "starspopup" and window["visible"] for window in before)
                design = json.loads((work / "enemy-design.json").read_text())
                assert design["fleet"] == 512 and design["player"] == 1 and design["design"] == 0
                assert design["hideCounts"] is True and design["summary"] is True
                assert design["showDamage"] is False and design["token"] is False
                assert json.loads((work / "year-2408-map.json").read_text())["turn"] == 8
            if scenario in ("fleet-split", "research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                checkpoints = [json.loads((work / name).read_text()) for name in (
                    "year-2408-start-orders.json", "year-2408-after-transfer.json",
                    "year-2408-split-cargo.json", "year-2408-before-split-all.json", "year-2408-split-orders.json")]
                assert [item["ownedFleetCount"] for item in checkpoints] == [9, 10, 10, 10, 11]
                fleets = [{fleet["id"]: fleet for fleet in item["fleets"]} for item in checkpoints]
                totals = [[sum(fleet["ships"][i] for fleet in item.values()) for i in range(16)] for item in fleets]
                assert all(total == totals[0] for total in totals), "splitting changed total ship counts"
                assert fleets[0][7]["ships"][2] == 3
                assert fleets[1][7]["ships"][2] == 2 and fleets[1][9]["ships"][2] == 1
                assert fleets[2][7]["cargo"][:4] == [0, 0, 0, 50]
                assert fleets[3][7]["cargo"] == fleets[2][7]["cargo"]
                final = fleets[4]
                assert final[7]["ships"][2] == final[9]["ships"][2] == final[10]["ships"][2] == 1
                assert final[7]["cargo"][:4] == final[10]["cargo"][:4] == [0, 0, 0, 25]
                assert [sum(final[i]["cargo"][j] for i in (7, 10)) for j in range(5)] == fleets[3][7]["cargo"]
                assert final[7]["orders"] == final[10]["orders"] == fleets[3][7]["orders"]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "fleet-split-map.json").read_text())["objects"] if obj["kind"] == 1}
                order = final[10]["orders"][1]
                assert (order["kind"], order["id"], order["task"], order["position"]) == (1, 8, 2, planets[8])
                for fleet_id in fleets[0].keys() - {7}:
                    assert final[fleet_id] == fleets[0][fleet_id]
                assert final[9] == fleets[1][9]
                assert all(item["turn"] == 8 and item["turnDone"] == 0 for item in checkpoints)
                assert all(item["research"] == checkpoints[0]["research"] and item["production"] == checkpoints[0]["production"] for item in checkpoints)
            if scenario in ("research-cycle", "oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                split = json.loads((work / "year-2408-split-orders.json").read_text())
                dragged = json.loads((work / "year-2408-drag-orders.json").read_text())
                routed = json.loads((work / "year-2408-scout-orders.json").read_text())
                checkpoint = json.loads((work / "year-2408-orders.json").read_text())
                assert ready[8]["turn"] == 8 and ready[8]["turnDone"] == 1
                assert checkpoint["turn"] == 8 and checkpoint["turnDone"] == 1
                assert split["filteredUnloads"] is False and checkpoint["filteredUnloads"] is True
                assert checkpoint["nextMessage"] == -1 and checkpoint["research"] == [1, 6, 30]
                assert split["research"] == [1, 6, 15]
                fleet_sets = [{fleet["id"]: fleet for fleet in item["fleets"]} for item in (split, dragged, routed, checkpoint)]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "waypoint-drag-map.json").read_text())["objects"] if obj["kind"] == 1}
                original, moved, scouts, final = fleet_sets
                for fleet_id in original.keys() - {7}:
                    assert moved[fleet_id] == original[fleet_id]
                for fleet_id in original.keys() - {8}:
                    assert scouts[fleet_id] == moved[fleet_id]
                assert final == scouts
                assert moved[7]["orders"][0] == original[7]["orders"][0]
                assert {key: value for key, value in moved[7].items() if key != "orders"} == {key: value for key, value in original[7].items() if key != "orders"}
                order = moved[7]["orders"][1]
                assert (order["kind"], order["id"], order["task"], order["position"]) == (1, 17, 2, planets[17])
                assert len(scouts[8]["orders"]) == 2 and scouts[8]["orders"][0] == original[8]["orders"][0]
                order = scouts[8]["orders"][1]
                assert (order["kind"], order["id"], order["task"], order["position"]) == (1, 9, 0, planets[9])
                assert {key: value for key, value in scouts[8].items() if key != "orders"} == {key: value for key, value in original[8].items() if key != "orders"}
                assert all(item["production"] == split["production"] and item["ownedFleetCount"] == 11 for item in (dragged, routed, checkpoint))
                assert all(item["research"] == split["research"] for item in (dragged, routed))
                for key in ("filteredFactories", "filteredMines", "planetFlags", "scannerMode"):
                    assert all(item[key] == split[key] for item in (dragged, routed, checkpoint))
                objects = json.loads((work / "wallaby-radiation-map.json").read_text())["objects"]
                assert any(obj["kind"] == 1 and obj["id"] == 5 and obj["scanSelected"] for obj in objects)
                assert json.loads((work / "wallaby-radiation.json").read_text()) == {"type": 5, "planet": 5, "environment": 2}
                assert json.loads((work / "radiation-benefit.json").read_text()) == {"type": 9, "kind": 8192, "item": 17}
                for name in ("wallaby-radiation", "radiation-benefit"):
                    windows = json.loads((work / f"{name}-hierarchy.json").read_text())["windows"]
                    assert any(window["class"] == "starspopup" and window["visible"] and window["captured"] for window in windows)
                windows = json.loads((work / "before-generate-2408-hierarchy.json").read_text())["windows"]
                assert not any(window["class"] == "starspopup" and window["visible"] for window in windows)
                for name in ("waypoint-dragged", "environment-popup", "research-benefit-popup"):
                    assert len([event for event in events if event["event"] == name]) == 1
                assert json.loads((work / "year-2409-map.json").read_text())["turn"] == 9
            if scenario in ("oxygen-colonization", "production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                previous = json.loads((work / "year-2409-start-orders.json").read_text())
                research = json.loads((work / "year-2409-research.json").read_text())
                cargo = json.loads((work / "year-2409-cargo.json").read_text())
                checkpoint = json.loads((work / "year-2409-orders.json").read_text())
                assert previous["turn"] == research["turn"] == cargo["turn"] == checkpoint["turn"] == 9
                assert previous["turnDone"] == research["turnDone"] == cargo["turnDone"] == 0 and checkpoint["turnDone"] == 1
                assert ready[9]["turn"] == 9 and ready[9]["turnDone"] == 1
                assert previous["research"] == [1, 6, 30] and research["research"] == cargo["research"] == checkpoint["research"] == [1, 3, 30]
                assert previous["fleets"] == research["fleets"]
                assert previous["filteredMiningLoads"] is False and checkpoint["filteredMiningLoads"] is True
                for key in ("production", "planetFlags", "ownedFleetCount", "scannerMode", "filteredFactories", "filteredMines", "filteredUnloads"):
                    assert all(item[key] == previous[key] for item in (research, cargo, checkpoint))
                original, loaded, final = [{fleet["id"]: fleet for fleet in item["fleets"]} for item in (previous, cargo, checkpoint)]
                for fleet_id in original.keys() - {9}:
                    assert loaded[fleet_id] == original[fleet_id]
                for fleet_id in original.keys() - {0, 9}:
                    assert final[fleet_id] == original[fleet_id]
                assert original[9]["cargo"][:4] == [0, 0, 0, 0]
                assert loaded[9]["cargo"][:4] == final[9]["cargo"][:4] == [0, 0, 0, 25]
                assert loaded[9]["cargo"][4] == original[9]["cargo"][4]
                assert loaded[9]["orders"] == original[9]["orders"]
                expected = dict(original[9], cargo=original[9]["cargo"][:])
                expected["cargo"][3] = 25
                assert loaded[9] == expected
                assert {key: value for key, value in final[9].items() if key != "orders"} == {key: value for key, value in loaded[9].items() if key != "orders"}
                assert {key: value for key, value in final[0].items() if key != "orders"} == {key: value for key, value in original[0].items() if key != "orders"}
                assert final[9]["ships"] == original[9]["ships"] and final[9]["cargo"] == loaded[9]["cargo"]
                assert final[0]["ships"] == original[0]["ships"] and final[0]["cargo"] == original[0]["cargo"]
                planets = {obj["id"]: obj["universe"] for obj in json.loads((work / "before-generate-2409-map.json").read_text())["objects"] if obj["kind"] == 1}
                for fleet_id, planet_id, task in ((9, 2, 2), (0, 4, 0)):
                    assert len(final[fleet_id]["orders"]) == 2
                    assert final[fleet_id]["orders"][0] == original[fleet_id]["orders"][0]
                    order = final[fleet_id]["orders"][1]
                    assert (order["kind"], order["id"], order["task"], order["position"]) == (1, planet_id, task, planets[planet_id])
                assert original[0]["orders"][1]["id"] == 2
                objects = json.loads((work / "oxygen-summary-map.json").read_text())["objects"]
                assert any(obj["kind"] == 1 and obj["id"] == 2 and obj["scanSelected"] for obj in objects)
                assert len([event for event in events if event["event"] == "oxygen-goto"]) == 1
                assert len([event for event in events if event["event"] == "mozart-waypoint-dragged"]) == 1
                assert json.loads((work / "year-2410-map.json").read_text())["turn"] == 10
            if scenario in ("production-templates", "battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                previous, queued, imported, accepted, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2410-start-orders.json", "year-2410-shaggy-queue.json", "year-2410-imported-template.json",
                    "year-2410-template-orders.json", "year-2410-orders.json")]
                assert all(item["turn"] == 10 for item in (previous, queued, imported, accepted, checkpoint))
                assert all(item["turnDone"] == 0 for item in (previous, queued, imported, accepted)) and checkpoint["turnDone"] == 1
                assert ready[10]["turn"] == 10 and ready[10]["turnDone"] == 1
                assert previous["filteredDismantles"] is False and checkpoint["filteredDismantles"] is True
                template = {"valid": True, "leftoverResearch": 1, "count": 2, "items": [{"item": 1, "quantity": 3}, {"item": 0, "quantity": 3}]}
                assert previous["defaultTemplate"] != template and queued["defaultTemplate"] == previous["defaultTemplate"]
                assert imported["defaultTemplate"] == accepted["defaultTemplate"] == checkpoint["defaultTemplate"] == template
                for key in ("fleets", "research", "ownedFleetCount", "scannerMode", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads"):
                    assert all(item[key] == previous[key] for item in (queued, imported, accepted, checkpoint))
                shaggy = [{"planet": 14, "kind": 1, "item": 1, "quantity": 3}, {"planet": 14, "kind": 1, "item": 0, "quantity": 3}]
                assert [item for item in queued["production"] if item["planet"] == 14] == shaggy
                assert imported["production"] == accepted["production"] == queued["production"]
                assert {item["planet"]: item["leftoverResearch"] for item in queued["planetFlags"]}[14] == 1
                assert imported["planetFlags"] == accepted["planetFlags"] == queued["planetFlags"]
                assert [item for item in checkpoint["production"] if item["planet"] == 14] == shaggy
                before = [item for item in previous["production"] if item["planet"] == 13]
                assert [item for item in checkpoint["production"] if item["planet"] == 13] == before[:1] + [
                    {"planet": 13, "kind": 2, "item": 2, "quantity": 1}, {"planet": 13, "kind": 2, "item": 3, "quantity": 1}] + before[1:]
                assert [item for item in checkpoint["production"] if item["planet"] not in (13, 14)] == [item for item in previous["production"] if item["planet"] not in (13, 14)]
                flags = {item["planet"]: item["leftoverResearch"] for item in checkpoint["planetFlags"]}
                assert flags[14] == 1 and flags[13] == 0
                for item in previous["planetFlags"]:
                    if item["planet"] not in (13, 14): assert flags[item["planet"]] == item["leftoverResearch"]
                windows = json.loads((work / "template-open-hierarchy.json").read_text())["windows"]
                by_id = {window["id"]: window for window in windows}
                dialog = next(window for window in windows if window["text"] == "Customize Production Templates" and window["visible"])
                owner = by_id[dialog["owner"]]
                assert dialog["enabled"] and owner["visible"] and not owner["enabled"]
                restored = json.loads((work / "template-owner-restored-hierarchy.json").read_text())["windows"]
                assert any(window["id"] == owner["id"] and window["enabled"] and window["visible"] for window in restored)
                assert not any(window["id"] == dialog["id"] for window in restored)
                closed = json.loads((work / "template-closed-hierarchy.json").read_text())["windows"]
                assert not any(window["id"] in (owner["id"], dialog["id"]) for window in closed)
                for name, planet_id in (("shaggy-dog-summary", 14), ("bloop-summary", 23)):
                    objects = json.loads((work / f"{name}-map.json").read_text())["objects"]
                    assert any(obj["kind"] == 1 and obj["id"] == planet_id and obj["scanSelected"] for obj in objects)
                for name in ("shaggy-dog-goto", "template-nested-open", "template-owner-restored", "bloop-goto"):
                    assert len([event for event in events if event["event"] == name]) == 1
                assert json.loads((work / "year-2411-map.json").read_text())["turn"] == 11
            if scenario in ("battle-cycle", "terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                previous, colonizer, troops, mines, research, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2411-start-orders.json", "year-2411-colonizer-cargo.json", "year-2411-troop-cargo.json",
                    "year-2411-mines.json", "year-2411-research.json", "year-2411-orders.json")]
                assert all(item["turn"] == 11 for item in (previous, colonizer, troops, mines, research, checkpoint))
                assert all(item["turnDone"] == 0 for item in (previous, colonizer, troops, mines, research)) and checkpoint["turnDone"] == 1
                assert ready[11]["turnDone"] == 1 and checkpoint["nextMessage"] == -1
                assert previous["research"][0] == 3 and previous["research"][2] == 30
                assert research["research"] == checkpoint["research"] == [3, 5, 30]
                for key in ("ownedFleetCount", "scannerMode", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles", "defaultTemplate", "planetFlags"):
                    assert all(item[key] == previous[key] for item in (colonizer, troops, mines, research, checkpoint))
                original, loaded_colonizer, loaded_troops, final = [{fleet["id"]: fleet for fleet in item["fleets"]} for item in (previous, colonizer, troops, checkpoint)]
                assert original.keys() == final.keys()
                for fleet_id in original:
                    assert final[fleet_id]["ships"] == original[fleet_id]["ships"]
                    assert final[fleet_id]["repeatOrders"] == original[fleet_id]["repeatOrders"]
                    assert final[fleet_id]["orders"][0] == original[fleet_id]["orders"][0]
                    if fleet_id not in (0, 1, 2, 4, 8, 11): assert final[fleet_id] == original[fleet_id]
                    if fleet_id not in (2, 11): assert final[fleet_id]["cargo"] == original[fleet_id]["cargo"]
                for fleet_id, loaded, amount in ((2, loaded_colonizer, 25), (11, loaded_troops, 210)):
                    assert original[fleet_id]["cargo"][:4] == [0, 0, 0, 0]
                    assert loaded[fleet_id]["cargo"][:4] == final[fleet_id]["cargo"][:4] == [0, 0, 0, amount]
                    assert final[fleet_id]["cargo"][4] == original[fleet_id]["cargo"][4]
                objects = {(obj["kind"], obj["id"]): obj["universe"] for obj in json.loads((work / "before-generate-2411-map.json").read_text())["objects"]}
                for fleet_id, kind, target, task in ((0, 1, 10, 0), (1, 1, 13, 5), (4, 1, 13, 0), (8, 2, 512, 0), (2, 1, 4, 2), (11, 1, 5, 1)):
                    assert len(final[fleet_id]["orders"]) == 2
                    order = final[fleet_id]["orders"][1]
                    assert (order["kind"], order["id"], order["task"], order["position"]) == (kind, target, task, objects[(kind, target)])
                assert final[11]["orders"][1]["transport"] == [{"action": 0, "quantity": 0}] * 3 + [{"action": 2, "quantity": 0}, {"action": 0, "quantity": 0}]
                before_queue = [item for item in previous["production"] if item["planet"] == 13]
                expected_queue = [{"planet": 13, "kind": 1, "item": 8, "quantity": 70}] + before_queue
                assert [item for item in mines["production"] if item["planet"] == 13] == expected_queue
                assert mines["production"] == research["production"] == checkpoint["production"]
                assert [item for item in checkpoint["production"] if item["planet"] != 13] == [item for item in previous["production"] if item["planet"] != 13]
                assert mines["fleets"] == research["fleets"] == checkpoint["fleets"]
                browser = json.loads((work / "technology-browser-hierarchy.json").read_text())["windows"]
                dialog = next(window for window in browser if window["dialog"] and window["visible"] and "Technology Browser" in window["text"])
                closed = json.loads((work / "technology-browser-closed-hierarchy.json").read_text())["windows"]
                assert not any(window["id"] == dialog["id"] for window in closed)
                battle_open, playing, end, rewound, stepped = [json.loads((work / name).read_text()) for name in (
                    "battle-open.json", "battle-playing.json", "battle-end.json", "battle-rewound.json", "battle-stepped.json")]
                assert battle_open["tokens"] > 0 and battle_open["lastStep"] > 0 and battle_open["battle"] != 65535
                assert all((item["battle"], item["tokens"], item["lastStep"]) == (battle_open["battle"], battle_open["tokens"], battle_open["lastStep"]) for item in (playing, end, rewound, stepped))
                assert not battle_open["playing"] and playing["playing"] and playing["step"] > battle_open["step"]
                assert not end["playing"] and end["step"] == end["lastStep"]
                assert not rewound["playing"] and rewound["step"] == -1 and stepped["step"] > -1
                windows = json.loads((work / "battle-open-hierarchy.json").read_text())["windows"]
                battle_dialog = next(window for window in windows if window["dialog"] and window["visible"] and window["text"] == "Battle VCR")
                closed = json.loads((work / "battle-closed-hierarchy.json").read_text())["windows"]
                assert not any(window["id"] == battle_dialog["id"] for window in closed)
                for name in ("technology-browser-open", "technology-browser-closed", "battle-open", "battle-playing", "battle-closed"):
                    assert len([event for event in events if event["event"] == name]) == 1
                assert json.loads((work / "year-2412-map.json").read_text())["turn"] == 12
            if scenario in ("terraform-cycle", "design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                previous, terraformed, queued, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2412-start-orders.json", "year-2412-terraform-orders.json", "year-2412-teamster-orders.json", "year-2412-orders.json")]
                assert all(item["turn"] == 12 for item in (previous, terraformed, queued, checkpoint))
                assert previous["turnDone"] == terraformed["turnDone"] == 0
                assert checkpoint["turnDone"] == ready[12]["turnDone"] == 1
                for key in ("fleets", "research", "ownedFleetCount", "scannerMode", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles", "defaultTemplate"):
                    assert all(item[key] == previous[key] for item in (terraformed, queued, checkpoint))
                original_slime = [item for item in previous["production"] if item["planet"] == 8]
                expected_slime = [{"planet": 8, "kind": 1, "item": 12, "quantity": 2}] + original_slime
                assert [item for item in terraformed["production"] if item["planet"] == 8] == expected_slime
                assert [item for item in terraformed["production"] if item["planet"] != 8] == [item for item in previous["production"] if item["planet"] != 8]
                original_stove = [item for item in previous["production"] if item["planet"] == 13]
                expected_stove = original_stove[:1] + [{"planet": 13, "kind": 2, "item": 3, "quantity": 2}] + original_stove[1:]
                assert [item for item in queued["production"] if item["planet"] == 13] == expected_stove
                assert [item for item in queued["production"] if item["planet"] != 13] == [item for item in terraformed["production"] if item["planet"] != 13]
                assert queued["production"] == checkpoint["production"]
                old_flags = {item["planet"]: item["leftoverResearch"] for item in previous["planetFlags"]}
                expected_flags = dict(old_flags)
                expected_flags[8] = 1
                expected_flags[13] = 0
                for item in (queued, checkpoint):
                    assert {flag["planet"]: flag["leftoverResearch"] for flag in item["planetFlags"]} == expected_flags
                salvage = json.loads((work / "salvage.json").read_text())
                objects = json.loads((work / "salvage-summary-map.json").read_text())["objects"]
                selected = next(obj for obj in objects if obj["kind"] == 8 and obj["id"] == salvage["id"])
                assert selected["scanSelected"] and selected["thingType"] == 1 and selected["name"].rstrip() == "Salvage"
                assert selected["universe"] == salvage["position"]
                probe = next(obj for obj in objects if obj["kind"] == 2 and obj["id"] == 8)
                assert probe["universe"] == salvage["position"]
                assert len(salvage["minerals"]) == 3 and salvage["minerals"][0] > 0 and salvage["minerals"][2] > 0
                opened = json.loads((work / "find-teamster-open-hierarchy.json").read_text())["windows"]
                dialog = next(window for window in opened if window["dialog"] and window["visible"] and window["text"] == "Find Planet or Fleet")
                entered = json.loads((work / "find-teamster-entered-hierarchy.json").read_text())["windows"]
                assert any(window["text"] == "Teamster #4" and window["parent"] == dialog["id"] for window in entered)
                closed = json.loads((work / "find-teamster-closed-hierarchy.json").read_text())["windows"]
                assert not any(window["id"] == dialog["id"] for window in closed)
                found = json.loads((work / "find-teamster-closed-map.json").read_text())["objects"]
                assert any(obj["kind"] == 2 and obj["id"] == 3 and obj["scanSelected"] for obj in found)
                probe_goto = json.loads((work / "armed-probe-goto-map.json").read_text())["objects"]
                assert any(obj["kind"] == 2 and obj["id"] == 8 and obj["scanSelected"] and obj["mainSelected"] for obj in probe_goto)
                for name in ("armed-probe-goto", "salvage-selected", "teamster-found"):
                    assert len([event for event in events if event["event"] == name and event["turn"] == 12]) == 1
                assert json.loads((work / "year-2413-map.json").read_text())["turn"] == 13
            if scenario in ("design-cycle", "automation-cycle", "report-cycle", "merge-cycle"):
                previous, cargo, transported, researched, miner, mines, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2413-start-orders.json", "year-2413-cargo.json", "year-2413-transport.json", "year-2413-research.json",
                    "year-2413-miner-queue.json", "year-2413-mines.json", "year-2413-orders.json")]
                assert all(item["turn"] == 13 for item in (previous, cargo, transported, researched, miner, mines, checkpoint))
                assert previous["turnDone"] == 0 and checkpoint["turnDone"] == ready[13]["turnDone"] == 1
                before_fleets = {fleet["id"]: fleet for fleet in previous["fleets"]}
                cargo_fleets = {fleet["id"]: fleet for fleet in cargo["fleets"]}
                expected_cargo = dict(before_fleets[0], cargo=before_fleets[0]["cargo"][:])
                expected_cargo["cargo"][3] = 210
                assert cargo_fleets[0] == expected_cargo
                assert {i: fleet for i, fleet in cargo_fleets.items() if i != 0} == {i: fleet for i, fleet in before_fleets.items() if i != 0}
                final_fleets = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                assert set(final_fleets) == set(before_fleets)
                for i in final_fleets:
                    assert final_fleets[i]["ships"] == before_fleets[i]["ships"]
                    assert final_fleets[i]["cargo"] == cargo_fleets[i]["cargo"]
                    assert final_fleets[i]["repeatOrders"] == before_fleets[i]["repeatOrders"]
                    if i not in (0, 8): assert final_fleets[i] == before_fleets[i]
                objects = json.loads((work / "before-generate-2413-map.json").read_text())["objects"]
                positions = {obj["id"]: obj["universe"] for obj in objects if obj["kind"] == 1}
                for i, planet in ((0, 8), (8, 13)):
                    assert len(final_fleets[i]["orders"]) == 2
                    order = final_fleets[i]["orders"][1]
                    assert order["kind"] == 1 and order["id"] == planet and order["position"] == positions[planet]
                unload = final_fleets[0]["orders"][1]
                assert unload["task"] == 1 and unload["transport"][3] == {"action": 2, "quantity": 0}
                assert all(action == {"action": 0, "quantity": 0} for j, action in enumerate(unload["transport"]) if j != 3)
                assert researched["research"] == checkpoint["research"] == [5, 2, 30]
                assert len(previous["designs"]) == 6 and len(checkpoint["designs"]) == 7
                assert checkpoint["designs"][:6] == previous["designs"]
                assert checkpoint["designs"][6] == {"index": 6, "hull": 21, "parts": [[1, 3, 1], [2, 1, 1], [128, 2, 1], [128, 2, 1]]}
                original_stove = [item for item in previous["production"] if item["planet"] == 13]
                expected_miner = original_stove[:1] + [{"planet": 13, "kind": 2, "item": 6, "quantity": 1}] + original_stove[1:]
                assert [item for item in miner["production"] if item["planet"] == 13] == expected_miner
                expected_mines = [{"planet": 13, "kind": 1, "item": 8, "quantity": 100}] + expected_miner
                assert [item for item in mines["production"] if item["planet"] == 13] == expected_mines
                assert mines["production"] == checkpoint["production"]
                assert checkpoint["planetFlags"] == previous["planetFlags"]
                for item in (cargo, transported, researched, miner, mines, checkpoint):
                    assert item["ownedFleetCount"] == previous["ownedFleetCount"]
                    assert [entry for entry in item["production"] if entry["planet"] != 13] == [entry for entry in previous["production"] if entry["planet"] != 13]
                    for key in ("scannerMode", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles", "defaultTemplate"):
                        assert item[key] == previous[key]
                assert json.loads((work / "year-2414-map.json").read_text())["turn"] == 14
                inspected = [event["productionSelection"] for event in events if event["event"] == "production-item-inspected" and event["turn"] == 13]
                assert inspected == list(range(len(expected_mines)))
            if scenario in ("automation-cycle", "report-cycle", "merge-cycle"):
                previous, queued, imported, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2414-start-orders.json", "year-2414-terraform-queue.json", "year-2414-imported-template.json", "year-2414-orders.json")]
                assert all(item["turn"] == 14 for item in (previous, queued, imported, checkpoint))
                assert previous["turnDone"] == 0 and checkpoint["turnDone"] == ready[14]["turnDone"] == 1
                oxygen = [item for item in previous["production"] if item["planet"] == 2]
                expected_queue = [{"planet": 2, "kind": 1, "item": 4, "quantity": 2}] + oxygen
                assert [item for item in queued["production"] if item["planet"] == 2] == expected_queue
                assert queued["production"] == imported["production"] == checkpoint["production"]
                template = {"valid": True, "leftoverResearch": 1, "count": len(expected_queue),
                            "items": [{"item": item["item"], "quantity": item["quantity"]} for item in expected_queue]}
                assert imported["defaultTemplate"] == checkpoint["defaultTemplate"] == template
                assert queued["defaultTemplate"] == previous["defaultTemplate"]
                assert [item for item in checkpoint["production"] if item["planet"] != 2] == [item for item in previous["production"] if item["planet"] != 2]
                flags = {item["planet"]: item["leftoverResearch"] for item in previous["planetFlags"]}
                flags[2] = 1
                assert {item["planet"]: item["leftoverResearch"] for item in checkpoint["planetFlags"]} == flags
                for key in ("fleets", "designs", "research", "scannerMode", "ownedFleetCount", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles"):
                    assert all(item[key] == previous[key] for item in (queued, imported, checkpoint))
                opened = json.loads((work / "terraform-template-open-hierarchy.json").read_text())["windows"]
                child = next(window for window in opened if window["dialog"] and window["visible"] and window["text"] == "Customize Production Templates")
                owner = next(window for window in opened if window["id"] == child["owner"])
                assert not owner["enabled"] and child["enabled"]
                closed = json.loads((work / "terraform-template-closed-hierarchy.json").read_text())["windows"]
                assert not any(window["id"] in (child["id"], owner["id"]) for window in closed)
                assert json.loads((work / "year-2415-map.json").read_text())["turn"] == 15
            if scenario in ("report-cycle", "merge-cycle"):
                previous, cargo, researched, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2415-start-orders.json", "year-2415-cargo.json", "year-2415-research.json", "year-2415-orders.json")]
                assert all(item["turn"] == 15 for item in (previous, cargo, researched, checkpoint))
                assert previous["turnDone"] == 0 and checkpoint["turnDone"] == ready[15]["turnDone"] == 1
                original = {fleet["id"]: fleet for fleet in previous["fleets"]}
                filled = {fleet["id"]: fleet for fleet in cargo["fleets"]}
                final = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                assert set(final) == set(original) == set(filled)
                expected = dict(original[6], cargo=original[6]["cargo"][:])
                expected["cargo"][3] = 210
                assert filled[6] == expected
                for i in final:
                    assert final[i]["ships"] == original[i]["ships"] and final[i]["repeatOrders"] == original[i]["repeatOrders"]
                    assert final[i]["cargo"] == filled[i]["cargo"]
                    if i not in (6, 8, 11): assert final[i] == original[i]
                objects = json.loads((work / "before-generate-2415-map.json").read_text())["objects"]
                positions = {obj["id"]: obj["universe"] for obj in objects if obj["kind"] == 1}
                for i, planet in ((8, 11), (6, 5), (11, 13)):
                    assert len(final[i]["orders"]) == 2
                    order = final[i]["orders"][1]
                    assert order["kind"] == 1 and order["id"] == planet and order["position"] == positions[planet]
                unload = final[6]["orders"][1]
                assert unload["task"] == 1 and unload["transport"][3] == {"action": 2, "quantity": 0}
                assert all(action == {"action": 0, "quantity": 0} for j, action in enumerate(unload["transport"]) if j != 3)
                for item in (cargo, researched, checkpoint):
                    for key in ("designs", "production", "research", "planetFlags", "defaultTemplate", "scannerMode", "ownedFleetCount", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles"):
                        assert item[key] == previous[key]
                report = json.loads((work / "value-report.json").read_text())
                assert report["turn"] == 15 and report["type"] == 0 and report["column"] == 4 and report["ascending"]
                assert report["planetIds"][0] == 5
                opened = json.loads((work / "value-report-open-hierarchy.json").read_text())["windows"]
                window = next(window for window in opened if window["visible"] and "Planet Summary Report" in window["text"])
                closed = json.loads((work / "value-report-closed-hierarchy.json").read_text())["windows"]
                assert not any(item["id"] == window["id"] for item in closed)
                research_window = json.loads((work / "propulsion-research-open-hierarchy.json").read_text())["windows"]
                assert any(item["visible"] and item["dialog"] and item["text"] == "Research" for item in research_window)
                assert json.loads((work / "year-2416-map.json").read_text())["turn"] == 16
            if scenario == "merge-cycle":
                previous, production, research, checkpoint = [json.loads((work / name).read_text()) for name in (
                    "year-2416-start-orders.json", "year-2416-production.json", "year-2416-research.json", "year-2416-orders.json")]
                assert all(item["turn"] == 16 for item in (previous, production, research, checkpoint))
                assert checkpoint["turnDone"] == ready[16]["turnDone"] == 1
                assert research["research"] == checkpoint["research"] == [2, 3, 30]
                original = {fleet["id"]: fleet for fleet in previous["fleets"]}
                final = {fleet["id"]: fleet for fleet in checkpoint["fleets"]}
                assert set(final) == set(original)
                objects = json.loads((work / "before-generate-2416-map.json").read_text())["objects"]
                positions = {(obj["kind"], obj["id"]): obj["universe"] for obj in objects}
                for i in final:
                    assert final[i]["ships"] == original[i]["ships"] and final[i]["cargo"] == original[i]["cargo"]
                    assert final[i]["repeatOrders"] == original[i]["repeatOrders"]
                    if i not in (4, 7): assert final[i] == original[i]
                assert len(final[4]["orders"]) == len(final[7]["orders"]) == 2
                assert final[4]["orders"][1]["kind"] == 1 and final[4]["orders"][1]["id"] == 5
                assert final[4]["orders"][1]["position"] == positions[1, 5]
                merge = final[7]["orders"][1]
                assert merge["kind"] == 2 and merge["id"] == 5 and merge["task"] == 4 and merge["position"] == positions[2, 5]
                expected_queue = [{"planet": 13, "kind": 1, "item": 1, "quantity": 60}, {"planet": 13, "kind": 1, "item": 0, "quantity": 60}]
                assert [item for item in production["production"] if item["planet"] == 13] == expected_queue
                assert production["production"] == research["production"] == checkpoint["production"]
                assert [item for item in checkpoint["production"] if item["planet"] != 13] == [item for item in previous["production"] if item["planet"] != 13]
                for key in ("designs", "planetFlags", "defaultTemplate", "scannerMode", "ownedFleetCount", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles"):
                    assert checkpoint[key] == previous[key]
                before_return, returned = [json.loads((work / name).read_text()) for name in ("year-2417-start-orders.json", "year-2417-orders.json")]
                assert before_return["turn"] == returned["turn"] == 17 and returned["turnDone"] == ready[17]["turnDone"] == 1
                original = {fleet["id"]: fleet for fleet in before_return["fleets"]}
                final = {fleet["id"]: fleet for fleet in returned["fleets"]}
                expected = dict(original[0], orders=original[0]["orders"][:])
                expected["orders"].append({"kind": 1, "id": 13, "position": positions[1, 13], "task": 0, "warp": 6})
                assert final[0] == expected
                assert {i: fleet for i, fleet in final.items() if i != 0} == {i: fleet for i, fleet in original.items() if i != 0}
                for key in ("designs", "production", "research", "planetFlags", "defaultTemplate", "scannerMode", "ownedFleetCount", "filteredFactories", "filteredMines", "filteredUnloads", "filteredMiningLoads", "filteredDismantles"):
                    assert returned[key] == before_return[key]
                assert json.loads((work / "year-2418-map.json").read_text())["turn"] == 18
    print(f"Native tutorial {scenario} checks passed (not full tutorial); artifacts: {artifacts}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
