import React from "react";
import clsx from "clsx";

const BambolaButton = ({
  onClick = () => {},
  isRilasciato = true,
  extraClasses,
}: {
  onClick?: () => void;
  isRilasciato?: boolean;
  extraClasses?: string;
}) => {
  const stateClass = isRilasciato
    ? "after:bg-green text-black after:border-green"
    : "after:bg-orange-500 text-white after:border-orange-500";

  const label = isRilasciato ? "RILASCIATO" : "TIRATO";

  return (
    <button
      onClick={onClick}
      className={clsx(
        "px-4 py-2 mx-2 z-10 font-medium transition-colors duration-200 cursor-pointer",
        stateClass,
        "focus:outline-none focus-visible:outline-4 focus-visible:outline-webkit-focus-ring-color",
        extraClasses,
        // Slanted border
        "relative after:content-[''] after:absolute",
        "after:top-0 after:left-0 after:right-0 after:bottom-0",
        "after:border-2 after:transform after:-skew-x-10 after:-z-10",
        "after:transition-colors after:duration-200"
      )}
    >
      {label}
    </button>
  );
};

export default BambolaButton;
