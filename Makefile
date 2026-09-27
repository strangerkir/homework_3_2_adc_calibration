
.PHONY: format

format:
		find main -type f \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} +

