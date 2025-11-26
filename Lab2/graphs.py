import matplotlib.pyplot as plt
import numpy as np

# Данные эксперимента (замени на реальные результаты из твоей программы)
threads = np.array([1, 2, 4, 8, 12, 16])
times = np.array([1.591309, 1.396585, 0.975815, 0.641451, 0.531157, 0.443973])  # примерные значения времени выполнения

# Вычисляем ускорение и эффективность
speedup = times[0] / times
efficiency = speedup / threads

# Настройка шрифта
plt.rcParams.update({'font.size': 10})

# 1. Время выполнения
plt.figure(figsize=(10, 5))
plt.plot(threads, times, marker='o', color='blue', linewidth=2)
plt.xlabel("Количество потоков")
plt.ylabel("Время (сек)")
plt.title("Время выполнения vs Количество потоков")
plt.grid(True, alpha=0.3)
plt.xticks(threads)
plt.tight_layout()
plt.show()

# 2. Ускорение
plt.figure(figsize=(10, 5))
plt.plot(threads, speedup, marker='o', color='green', linewidth=2)
plt.xlabel("Количество потоков")
plt.ylabel("Ускорение")
plt.title("Ускорение vs Количество потоков")
plt.grid(True, alpha=0.3)
plt.xticks(threads)
plt.tight_layout()
plt.show()

# 3. Эффективность
plt.figure(figsize=(10, 5))
plt.plot(threads, efficiency, marker='o', color='red', linewidth=2)
plt.xlabel("Количество потоков")
plt.ylabel("Эффективность")
plt.title("Эффективность vs Количество потоков")
plt.ylim(0, 1.1)
plt.grid(True, alpha=0.3)
plt.xticks(threads)
plt.tight_layout()
plt.show()
