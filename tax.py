import sys


# https://j.eastday.com/p/1633410715038073
housing = 4342 * 0.5 + 3102 * 0.5

# http://m.sh.bendibao.com/zffw/237665.html
insurance = 31014 * 0.105

std_deduction = 5000


# https://jcc.bjmu.edu.cn/docs/20220104113607391688.pdf
def income_tax(income):
    tax_table = (
           #(upper,       rate,  deduction),
            (36000,       0.03,  0        ),
            (144000,      0.10,  2520     ),
            (300000,      0.20,  16920    ),
            (420000,      0.25,  31920    ),
            (660000,      0.30,  52920    ),
            (960000,      0.35,  85920    ),
            (sys.maxsize, 0.45,  181920   ))

    for upper, rate, deduction in tax_table:
        if income <= upper:
            return income * rate - deduction
    else:
        raise Exception("wtf")


# tax rate per bonus / 12, valid till 2023 end of year
def beneficial_tax(bonus):
    tax_table = (
            #(upper,       rate,  deduction),
             (3000,        0.03,  0        ),
             (12000,       0.10,  210      ),
             (25000,       0.20,  1410     ),
             (35000,       0.25,  2660     ),
             (55000,       0.30,  4410     ),
             (80000,       0.35,  7160     ),
             (sys.maxsize, 0.45,  15160    ))

    bonus_per_month = bonus / 12.0
    for upper, rate, deduction in tax_table:
        if bonus_per_month <= upper:
            return bonus * rate - deduction
    else:
        raise Exception("wft")


def get_inputs():
    salary = float(input('salary per month: '))
    allowance = 1500
    s = input('allowance per month ({}): '.format(allowance))
    if s:
        allowance = float(s)
    deduction = 0
    s = input('child, housing loan, elderly support deductions({}): '.format(
              deduction))
    if s:
        deduction = float(s)

    bonus={}
    while True:
        s = input('bonus month ([enter] if no more): ')
        if not s:
            break
        try:
            month = int(s)
            if month < 1 or month > 12:
                raise ValueError()
        except ValueError:
            print('-- invalid month, try again')
            continue
        value = None
        while not value:
            try:
                value = float(input('  amount: '))
            except ValueError:
                print('-- invalid value, try again')
        bonus[month] = value

    beneficial_month = None
    if bonus:
        while not beneficial_month:
            s = input('beneficial month ([enter] to ignore): ')
            if not s:
                beneficial_month = -1
                break
            try:
                beneficial_month = int(s)
                if beneficial_month < 1 or beneficial_month > 12 or \
                   beneficial_month not in bonus:
                       beneficial_value = None
                       raise ValueError()
            except ValueError:
                print('-- beneficial month not in bonus monthes, try again')

    return salary+allowance, deduction+std_deduction, bonus, beneficial_month


def debug(s):
    if len(sys.argv) > 1 and sys.argv[1] == 'debug':
        print(s)


income_per_month, total_deduction, bonus, beneficial_month = get_inputs()
net_month_income, month_tax = {}, {}

total_income, total_tax = 0.0, 0.0
for month in range(1, 13):
    debug('========== month {} =========='.format(month))
    income_this_month = income_per_month - housing - insurance - total_deduction
    debug('income before tax: {:.02f}'.format(income_this_month))
    net_beneficial_bonus, beneficial_bonus_tax = 0.0, 0.0
    if month in bonus:
        debug('bonus: {:.02f}'.format(bonus[month]))
        if month == beneficial_month:
            beneficial_bonus_tax = beneficial_tax(bonus[month])
            net_beneficial_bonus = bonus[month] - beneficial_bonus_tax
            debug('bonus tax: {:.02f}'.format(beneficial_bonus_tax))
        else:
            income_this_month += bonus[month]
            debug('bonus as salary, no benificial')
    total_income += income_this_month
    income_tax_this_month = income_tax(total_income)
    debug('salary tax: {:.02f}'.format(income_tax_this_month))
    month_tax[month] = income_tax_this_month - total_tax
    total_tax = income_tax_this_month
    net_month_income[month] = income_this_month - month_tax[month] + \
                              net_beneficial_bonus + total_deduction
    month_tax[month] += beneficial_bonus_tax
    debug('net pay: {:.02f}, tax: {:.02f}'.format(
          net_month_income[month], month_tax[month]))

total_net_pay, total_tax = 0.0, 0.0
print('month\tnet pay\t\ttax')
for month in range(1, 13):
    print('{}:\t{:.02f}\t{:.02f}'.format(
          month, net_month_income[month], month_tax[month]))
    total_net_pay += net_month_income[month]
    total_tax += month_tax[month]
print('-----------------------------------')
print('sum:\t{:.02f}\t{:.02f}'.format(total_net_pay, total_tax))
