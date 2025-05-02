% MATLAB Script for NMSE v. SNR Plot (used in Final Report)
% Virginia Tech ECE Department
% Author: Christopher Boerner

% Data
SNR_dB_range       = [ 0  4  8 12 16 20];
test_NMSE_avg      = [-13.5085422  -17.07663006 -21.24854471 -24.75378508 -28.91156788 -28.65034104];
AR_test_NMSE_avg   = [ -3.75639805  -5.48333268   -6.87055535  -5.85575509  -5.88480373  -6.75513861];
SOC_test_NMSE_avg  = [-20.30350     -23.50448    -26.05625    -27.74150    -28.68467    -29.13668   ];

% Create figure
figure; hold on; grid on;
set(gcf,'Color','w');

% Plot each curve
p1 = plot(SNR_dB_range, test_NMSE_avg,  '-*', 'Color',[0 0.4470 0.7410], ...
          'LineWidth',1.5,'MarkerSize',8, 'DisplayName','Channel Prediction with Python ESN');
p2 = plot(SNR_dB_range, AR_test_NMSE_avg, '-o', 'Color',[0.4660 0.6740 0.1880], ...
          'LineWidth',1.5,'MarkerSize',8, 'DisplayName','Channel Prediction with AR model');
p3 = plot(SNR_dB_range, SOC_test_NMSE_avg,'-s', 'Color',[0.4940 0.1840 0.5560], ...
          'LineWidth',1.5,'MarkerSize',8, 'DisplayName','Channel Prediction with FPGA cSoC ESN');

% Axes labels and title
xlabel('SNR (dB)','FontSize',12);
ylabel('NMSE (dB)','FontSize',12);
xlim([min(SNR_dB_range) max(SNR_dB_range)]);
ylim([-30  0]);  % adjust as needed
set(gca,'XTick',SNR_dB_range,'YTick',-30:2:0,'FontSize',11);

% Legend
legend('Location','southwest','FontSize',10,'Box','off');

% Optionally add a light background grid
ax = gca;
ax.GridLineStyle = '-';
ax.GridAlpha = 0.3;

hold off;
