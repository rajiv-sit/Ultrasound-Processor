import sys
import xml.etree.ElementTree as ET


def main() -> int:
    if len(sys.argv) != 3:
        print("usage: check_coverage.py <cobertura_xml> <min_percent>")
        return 2

    report_path = sys.argv[1]
    min_percent = float(sys.argv[2])

    tree = ET.parse(report_path)
    root = tree.getroot()

    line_rate = root.attrib.get("line-rate")
    if line_rate is None:
        print("coverage xml missing line-rate attribute")
        return 2

    measured = float(line_rate) * 100.0
    print(f"line coverage: {measured:.2f}% (required: {min_percent:.2f}%)")
    if measured < min_percent:
        print("coverage threshold not met")
        return 1

    print("coverage threshold met")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
