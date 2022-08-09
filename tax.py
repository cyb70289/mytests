import datetime
import sys


# highest housing + insurances, shanghai,  adjusted at middle of year
top_housing_insurance = (
        #Year Jan~Jun, Jul~Dec
        (2021, 6303.90, 6978.60),
        (2022, 6978.60, 7691.74)
)

# lowest income per month to pay tax, 2022
tax_lowest = 5000


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

    if income <= tax_lowest:
        return 0

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
    def get_month(hint):
        month = None
        while month is None:
            s = input(hint)
            if not s:
                return None
            try:
                month = int(s)
                if month < 1 or month > 12:
                    month = None
                    raise ValueError()
            except ValueError:
                print('-- invalid month, try again')
        return month

    def get_float(hint, default=None):
        value = None
        while value is None:
            s = input(hint)
            if (default is not None) and (not s):
                return default
            try:
                value = float(s)
            except ValueError:
                print('-- invalid value, try again')
        return value 

    def get_housing_insurance():
        year = datetime.date.today().year
        if len(sys.argv) == 2:
            try:
                year = int(sys.argv[1])
            except:
                pass
        for x in top_housing_insurance:
            if x[0] == year:
                housing_insurance = list(x[1:])
                break
        else:
            housing_insurance = list(top_housing_insurance[-1, 1:])
        v = housing_insurance[0]
        housing_insurance[0] = \
                get_float('housing + insurance, Jan ~ Jun ({:.02f}): '.format(v), v)
        v = housing_insurance[1]
        housing_insurance[1] = \
                get_float('housing + insurance, Jul ~ Dec ({:.02f}): '.format(v), v)
        return housing_insurance

    salary = get_float('salary per month: ')
    allowance = get_float('allowance per month (1500): ', 1500)
    housing_insurance = get_housing_insurance()
    deduction = \
        get_float('child, housing loan, elderly support deductions(0): ', 0)

    bonus={}
    while True:
        month = get_month('bonus month ([enter] if done): ')
        if month is None: break
        bonus[month] = get_float('  amount: ')

    beneficial_month = None
    if bonus:
        while beneficial_month is None:
            beneficial_month = \
                get_month('beneficial month ([enter] to ignore): ')
            if beneficial_month is None:
                beneficial_month = -1
                break
            elif beneficial_month not in bonus:
                beneficial_month = None
                print('-- beneficial month not in bonus monthes, try again')

    cash_reward = {i:0 for i in range(1, 13)}
    while True:
        month = get_month('cash reward month ([enter] if done): ')
        if month is None: break
        cash_reward[month] = get_float('  amount: ')

    return salary+allowance, housing_insurance, deduction+tax_lowest, \
           bonus, beneficial_month, cash_reward


def debug(s):
    if len(sys.argv) > 1 and sys.argv[1] == 'debug':
        print(s)


income_per_month, housing_insurance, total_deduction, bonus, \
    beneficial_month, cash_reward = get_inputs()
net_month_income, month_tax = {}, {}

total_income, total_tax = 0.0, 0.0
for month in range(1, 13):
    debug('========== month {} =========='.format(month))
    income_this_month = income_per_month + cash_reward[month] \
                        - housing_insurance[month >= 7] - total_deduction
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
    net_month_income[month] = income_this_month - month_tax[month] \
                              + net_beneficial_bonus + total_deduction
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
