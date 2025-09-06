#!/bin/bash
sum=0
count=0

for num in "$@"; do
    # Удаляем все нечисловые символы
    clean_num=$(echo "$num" | tr -cd '0-9')
    if [ -n "$clean_num" ]; then
        sum=$((sum + clean_num))
        count=$((count + 1))
    fi
done

if [ $count -eq 0 ]; then
    echo "Нет чисел для вычисления"
    exit 1
fi

average=$((sum / count))
echo "Количество чисел: $count"
echo "Сумма чисел: $sum"
echo "Среднее арифметическое: $average"
