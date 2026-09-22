import argparse
import math
from itertools import combinations


def integer_square_root(value: int) -> int | None:
    if value < 0:
        return None

    root = math.isqrt(value)
    return root if root * root == value else None


def main():
    parser = argparse.ArgumentParser(
        description="Check whether sums of all number pairs are perfect squares."
    )
    parser.add_argument(
        "numbers",
        type=int,
        nargs="+",
        help="Integer numbers to check"
    )

    args = parser.parse_args()
    numbers = args.numbers

    all_squares = True

    for a, b in combinations(numbers, 2):
        total = a + b
        root = integer_square_root(total)

        if root is not None:
            print(f"{a} + {b} = {total} = {root}^2")
        else:
            print(f"{a} + {b} = {total}  NOT A SQUARE")
            all_squares = False

    print()
    if all_squares:
        print("ALL pair sums are perfect squares.")
    else:
        print("NOT all pair sums are perfect squares.")


if __name__ == "__main__":
    main()