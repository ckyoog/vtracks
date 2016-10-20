#!/usr/bin/perl -w
#!/opt/bin/perl -w

# Generate regular expression for matching numbers which are
# (greater than (gt)) or (greater than or equal to (ge)) a
# given number.
#
# The basic mind is, for a given number which has x digit, Dx Dx-1 Dx-2 ... D1, e.g.
# 92185 has 5 digit, D5 = 9, D4 = 2, D3 = 1, D2 = 8, D1 = 5.
# The expected regular expression is something like the following,
# [1-9]{x,}|[(Dx + 1)-9]\d{x-1}|Dx[(Dx-1 + 1)-9]\d{x-2}|...|DxDx-1 ... [(D1 + 1)-9]
# ^^^^^^ ^^^^        ^^^^^^   ^^  ^          ^^^^^^   ^^   ^           ^        ^^^
# Parts pointed by ^ is regular expression characters, the others are variables.

sub get_range
{
    my ($start) = @_;
    my $range;
    if ($start != 9) {
        $range = "[$start-9]";
    } else {
        $range = "$start"
    }
    return $range;
}

#my $DIGIT_CLASS = "\\d";   sed, grep don't support \d
my $DIGIT_CLASS = "[0-9]";

# the origin mind, simple and straight algorithm
sub level0_0
{
    my ($num, $ge) = @_;
    my $strlen = length($num);
    my $regstr = "[1-9]$DIGIT_CLASS\{$strlen,}";
    my $prefix = "|";
    for my $i (1..$strlen) {
        my $l = $strlen - $i;
        my $d = substr($num, $i - 1, 1);
        if ($d == 9 && !($l == 0 && $ge)) {
            goto loop_end;
        }
        my $start = $l == 0 && $ge ? $d : $d + 1;
        my $suffix = "$DIGIT_CLASS\{$l}";
        $regstr = join('', $regstr, $prefix, get_range($start), $suffix);
loop_end:
        $prefix = join('', $prefix, $d);
    }
    return "($regstr)";
}

# Smarter than level0_0, no ending suffix
sub level1_0
{
    my ($num, $ge) = @_;
    my $strlen = length($num);
    my $regstr = "[1-9]$DIGIT_CLASS\{$strlen,}";
    my $prefix = "|";
    for my $i (1..$strlen) {
        my $l = $strlen - $i;
        my $d = substr($num, $i - 1, 1);
        if ($d == 9 && !($l == 0 && $ge)) {
            goto loop_end;
        }
        my $start = $l == 0 && $ge ? $d : $d + 1;
        my $suffix;
        if ($l != 0) {
            $suffix = "$DIGIT_CLASS\{$l}";
        } else {
            $suffix = "";
        }
        $regstr = join('', $regstr, $prefix, get_range($start), $suffix);
loop_end:
        $prefix = join('', $prefix, $d);
    }
    return "($regstr)";
}

# Optimized level1_0, no need to check for suffix in loop over and over again
sub level1_1
{
    my ($num, $ge) = @_;
    my $strlen = length($num);
    my $regstr = "";
    my $prefix = "";
    my ($d, $suffix, $next_prefix) = (0, "$DIGIT_CLASS\{$strlen,}", "|");
    for my $i (1..$strlen) {
        if ($d != 9) {
            $regstr = join('', $regstr, $prefix, get_range($d + 1), $suffix);
        }
        my $l = $strlen - $i;
        $d = substr($num, $i - 1, 1);
        $prefix = $next_prefix;
        $suffix = "${DIGIT_CLASS}\{$l}";
        $next_prefix = join('', $next_prefix, $d);
    }
    if ($d != 9 || $ge) {
        $regstr = join('', $regstr, $prefix, get_range($ge ? $d : $d + 1));
    }
    return "($regstr)";
}

# Smarter than level1_0, no ending suffix and '{1}'
sub level2_0
{
    my ($num, $ge) = @_;
    my $strlen = length($num);
    my $regstr = "[1-9]$DIGIT_CLASS\{$strlen,}";
    my $prefix = "|";
    for my $i (1..$strlen) {
        my $l = $strlen - $i;
        my $d = substr($num, $i - 1, 1);
        if ($d == 9 && !($l == 0 && $ge)) {
            goto loop_end;
        }
        my $start = $l == 0 && $ge ? $d : $d + 1;
        my $suffix;
        if ($l != 0) {
            $suffix = $l == 1 ? "$DIGIT_CLASS" : "$DIGIT_CLASS\{$l}";
        } else {
            $suffix = "";
        }
        $regstr = join('', $regstr, $prefix, get_range($start), $suffix);
loop_end:
        $prefix = join('', $prefix, $d);
    }
    return "($regstr)";
}

# Optimized level2_0, no need to check for suffix in loop over and over again
sub level2_1
{
    my ($num, $ge) = @_;
    my $strlen = length($num);
    my $regstr = "";
    my $prefix = "";
    my ($d, $suffix, $next_prefix) = (0, "$DIGIT_CLASS\{$strlen,}", "|");

    # The first part of the regular expression.
    $regstr = join('', $regstr, $prefix, get_range($d + 1), $suffix);
    $d = 9; #skip next make-regstr

    # The middle digit (may not exist).
    for my $i (1..($strlen - 1)) {
        if ($d != 9) {
            $regstr = join('', $regstr, $prefix, get_range($d + 1), $suffix);
        }
        my $l = $strlen - $i;
        $d = substr($num, $i - 1, 1);
        $prefix = $next_prefix;
        $suffix = "${DIGIT_CLASS}\{$l}";
        $next_prefix = join('', $next_prefix, $d);
    }

    # The second digit to last, which is worked out in above loop (may not exist).
    if ($d != 9) {
        $regstr = join('', $regstr, $prefix, get_range($d + 1), $DIGIT_CLASS);
    }

    # The last digit.
    $d = substr($num, $strlen - 1, 1);
    $prefix = $next_prefix;
    if ($d != 9 || $ge) {
        $regstr = join('', $regstr, $prefix, get_range($ge ? $d : $d + 1));
    }

    return "($regstr)";
}

print(level0_0(@ARGV)."\n");
print(level1_0(@ARGV)."\n");
print(level1_1(@ARGV)."\n");
print(level2_0(@ARGV)."\n");
print(level2_1(@ARGV)."\n");
