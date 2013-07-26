function plot_eeg(eeg_data,eeg)
% PLOT_EEG display EEG/ERP/EP data.
%   PLOT_EEG(eeg_data,eeg),where
%   eeg_data    - [MxN] matrix, M-number of samples,N-number of channels
%   eeg         - structure with vertical scale, data type and electrode info
%
% Part of Neuroscan's SCAN 4.3 package

% Function requires at least one input argument
if nargin<1
    error('Not enough input arguments.');
end

% Process arguments
if nargin<2
    scale= 128;
    type = 'EEG';
else
    scale= eeg.vscale;
    type = eeg.type;    
end

figure(1);
clf;
[m,n]=size(eeg_data);
h = axes('Position',[0 0 1 1],'Visible','off');

switch type
case 'EEG'
    axes('Position',[0.05 0.02 0.9 0.96]);
    axis off;
    hold on;
    axis([0 m -scale*(n+2) scale]);
    for i=1:n
        plot(eeg_data(:,i)-scale*i);
        text(-100,-i*scale,eeg.elec(i).name,'FontSize',10);
    end
otherwise
    for i=1:n
        xmin = eeg.elec(i).xmin;
        xmax = eeg.elec(i).xmax;
        ymin = eeg.elec(i).ymin+0.1;
        ymax = eeg.elec(i).ymax+0.1;
        axes('Position',[xmin 1-ymin xmax-xmin ymax-ymin]);
        % axis off;
        hold on;
        axis([0 m -10 10]);
        plot(-eeg_data(:,i));
        % grid on;
        title(eeg.elec(i).name);
    end
end

set(gcf,'CurrentAxes',h);
axis xy
text(0.4,0.98,strcat(type,' from ACQUIRE'),'FontSize',16);
